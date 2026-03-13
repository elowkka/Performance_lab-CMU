/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

#define src_to_dst                 \
*dst_p = *src_p; src_p += dim; ++dst_p \

#define src_to_dst8     \
src_to_dst; src_to_dst; \
src_to_dst; src_to_dst; \
src_to_dst; src_to_dst; \
src_to_dst; src_to_dst

#define src2_to_sum(x, y)                                    \
sum_p->red = (unsigned)(src_##x->red + src_##y->red);        \
sum_p->green = (unsigned)(src_##x->green + src_##y->green);  \
sum_p->blue = (unsigned)(src_##x->blue + src_##y->blue)      

#define src3_to_sum                                                    \
sum_p->red = (unsigned)(src_1->red + src_2->red + src_3->red);         \
sum_p->green = (unsigned)(src_1->green + src_2->green + src_3->green); \
sum_p->blue = (unsigned)(src_1->blue + src_2->blue + src_3->blue)

#define sum2_to_dst(x, y, z)                                               \
dst_ij->red = (unsigned short)((acc_##x->red + acc_##y->red) / (z));       \
dst_ij->green = (unsigned short)((acc_##x->green + acc_##y->green) / (z)); \
dst_ij->blue = (unsigned short)((acc_##x->blue + acc_##y->blue) / (z))

#define sum3_to_dst(x)                                                                \
dst_ij->red = (unsigned short)((acc_1->red + acc_2->red + acc_3->red) / (x));         \
dst_ij->green = (unsigned short)((acc_1->green + acc_2->green + acc_3->green) / (x)); \
dst_ij->blue = (unsigned short)((acc_1->blue + acc_2->blue + acc_3->blue) / (x))

#define updata_src2_to_sum \
sum_p += 3;                \
++src_1;                   \
src_2 = src_1 + 1

#define updata_src3_to_sum \
sum_p += 3;                \
++src_1;                   \
src_2 = src_1 + 1;         \
src_3 = src_1 + 2

#define updata_sum2_to_dst \
++dst_ij;                  \
acc_1 += 3;                \
acc_2 = acc_1 + 1

#define updata_sum3_to_dst \
++dst_ij;                  \
acc_1 += 3;                \
acc_2 = acc_1 + 1;         \
acc_3 = acc_1 + 2

 /*
  * Please fill in the following team struct
  */
team_t team = {
	"许贺韬",              /* Team name */

	"20300750034",           /* First member full name */
	"20300750034@fudan.edu.cn",           /* First member email address */

	"",                   /* Second member full name (leave blank if none) */
	""                    /* Second member email addr (leave blank if none) */
};

/***************
 * ROTATE KERNEL
 ***************/

 /******************************************************
  * Your different versions of the rotate kernel go here
  ******************************************************/

  /*
   * naive_rotate - The naive baseline version of rotate
   */
char naive_rotate_descr[] = "naive_rotate: Naive baseline implementation";
void naive_rotate(int dim, pixel* src, pixel* dst)
{
	int i, j;

	for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
			dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
}

/*
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel* src, pixel* dst)
{
	int i, j;
	int i_max = dim - 31;
	for (i = 0; i < i_max; i += 32) {
		for (j = dim - 1; j >= 0; j -= 1) {
			pixel* dst_p = dst + RIDX(dim - 1 - j, i, dim);
			pixel* src_p = src + RIDX(i, j, dim);
			src_to_dst8;
			src_to_dst8;
			src_to_dst8;
			src_to_dst8;
		}
	}

	for (; i < dim; ++i)
		for (j = 0; j < dim; ++j)
			dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
}

/*********************************************************************
 * register_rotate_functions - Register all of your different versions
 *     of the rotate kernel with the driver by calling the
 *     add_rotate_function() for each test function. When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_rotate_functions()
{
	add_rotate_function(&rotate, rotate_descr);
	add_rotate_function(&naive_rotate, naive_rotate_descr);
	/* ... Register additional test functions here */
}


/***************
 * SMOOTH KERNEL
 **************/

 /***************************************************************
  * Various typedefs and helper functions for the smooth function
  * You may modify these any way you like.
  **************************************************************/

  /* A struct used to compute averaged pixel value */
typedef struct {
	int red;
	int green;
	int blue;
	int num;
} pixel_sum;

/* Compute min and max of two integers, respectively */
static int min(int a, int b) { return (a < b ? a : b); }
static int max(int a, int b) { return (a > b ? a : b); }

/*
 * initialize_pixel_sum - Initializes all fields of sum to 0
 */
static void initialize_pixel_sum(pixel_sum* sum)
{
	sum->red = sum->green = sum->blue = 0;
	sum->num = 0;
	return;
}

/*
 * accumulate_sum - Accumulates field values of p in corresponding
 * fields of sum
 */
static void accumulate_sum(pixel_sum* sum, pixel p)
{
	sum->red += (int)p.red;
	sum->green += (int)p.green;
	sum->blue += (int)p.blue;
	sum->num++;
	return;
}

/*
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel
 */
static void assign_sum_to_pixel(pixel* current_pixel, pixel_sum sum)
{
	current_pixel->red = (unsigned short)(sum.red / sum.num);
	current_pixel->green = (unsigned short)(sum.green / sum.num);
	current_pixel->blue = (unsigned short)(sum.blue / sum.num);
	return;
}

/*
 * avg - Returns averaged pixel value at (i,j)
 */
static pixel avg(int dim, int i, int j, pixel* src)
{
	int ii, jj;
	pixel_sum sum;
	pixel current_pixel;

	initialize_pixel_sum(&sum);
	for (ii = max(i - 1, 0); ii <= min(i + 1, dim - 1); ii++)
		for (jj = max(j - 1, 0); jj <= min(j + 1, dim - 1); jj++)
			accumulate_sum(&sum, src[RIDX(ii, jj, dim)]);

	assign_sum_to_pixel(&current_pixel, sum);
	return current_pixel;
}

/******************************************************
 * Your different versions of the smooth kernel go here
 ******************************************************/

 /*
  * naive_smooth - The naive baseline version of smooth
  */
char naive_smooth_descr[] = "naive_smooth: Naive baseline implementation";
void naive_smooth(int dim, pixel* src, pixel* dst)
{
	int i, j;

	for (i = 0; i < dim; i++)
		for (j = 0; j < dim; j++)
			dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}

/*
 * smooth - Your current working version of smooth.
 * IMPORTANT: This is the version you will be graded on
 */
typedef struct {
	unsigned red;
	unsigned green;
	unsigned blue;
} pixel_s;

char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel* src, pixel* dst) {
	pixel* dst_ij;
	pixel_s* acc = (pixel_s*)malloc(3 * dim * sizeof(pixel_s));
	pixel_s* j_end = acc + 3 * dim - 6;
	pixel_s* sum_p, * acc_1, * acc_2, * acc_3;
	pixel* src_1, * src_2, * src_3, * i_end = src + dim * dim - 2;
	int i = 2;

	{
		//先计算acc数组的第一行
		sum_p = acc;
		src_1 = src - 1;
		src_2 = src;
		src_3 = src + 1;
		src2_to_sum(2, 3);
		while (sum_p < j_end) {
			updata_src3_to_sum;
			src3_to_sum;
		}
		updata_src2_to_sum;
		src2_to_sum(1, 2);

		//处理左上角元素
		sum_p = acc + 1;
		++src_1;
		src_2 = src_1 + 1;
		src_3 = src_1 + 2;
		src2_to_sum(2, 3);
		dst_ij = dst;
		acc_1 = acc;
		acc_2 = acc + 1;
		sum2_to_dst(1, 2, 4);
		//处理上方边界
		while (acc_1 < j_end) {
			updata_src3_to_sum;
			src3_to_sum;
			updata_sum2_to_dst;
			sum2_to_dst(1, 2, 6);
		}
		//处理右上角元素
		updata_src2_to_sum;
		src2_to_sum(1, 2);
		updata_sum2_to_dst;
		sum2_to_dst(1, 2, 4);
	}

	while (src_1 < i_end) {
		//处理左侧边界
		sum_p = acc + i;
		i = i == 2 ? 0 : i + 1;
		++src_1;
		src_2 = src_1 + 1;
		src_3 = src_1 + 2;
		src2_to_sum(2, 3);
		++dst_ij;
		acc_1 = acc;
		acc_2 = acc + 1;
		acc_3 = acc + 2;
		sum3_to_dst(6);
		//处理中心元素
		while (acc_1 < j_end) {
			updata_src3_to_sum;
			src3_to_sum;
			updata_sum3_to_dst;
			sum3_to_dst(9);
		}
		//处理右侧边界
		updata_src2_to_sum;
		src2_to_sum(1, 2);
		updata_sum3_to_dst;
		sum3_to_dst(6);
	}

	{
		//处理左下角元素
		++dst_ij;
		acc_1 = acc + (dim - 2) % 3;
		acc_2 = acc + (dim - 1) % 3;
		sum2_to_dst(1, 2, 4);
		//处理下方边界
		while (acc_1 < j_end) {
			++dst_ij;
			acc_1 += 3;
			acc_2 += 3;
			sum2_to_dst(1, 2, 6);
		}
		//处理右下角元素
		++dst_ij;
		acc_1 += 3;
		acc_2 += 3;
		sum2_to_dst(1, 2, 4);
	}

	free(acc);
}

/*********************************************************************
 * register_smooth_functions - Register all of your different versions
 *     of the smooth kernel with the driver by calling the
 *     add_smooth_function() for each test function.  When you run the
 *     driver program, it will test and report the performance of each
 *     registered test function.
 *********************************************************************/

void register_smooth_functions() {
	add_smooth_function(&smooth, smooth_descr);
	add_smooth_function(&naive_smooth, naive_smooth_descr);
	/* ... Register additional test functions here */
}

