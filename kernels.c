/********************************************************
 * Kernels to be optimized for the CS:APP Performance Lab
 ********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "defs.h"

#define row1(x) \
dst->x = ((src-1)->x + (src->x) + (src+1)->x + (src+dim-1)->x + (src+dim)->x + (src+dim+1)->x)/6\

#define row_last(x) \
dst->x = ((src-1)->x + (src->x) + (src+1)->x + (src-dim-1)->x + (src-dim)->x + (src-dim+1)->x)/6\

#define sum2(x) \
sum2_##x = (src-dim)->x + src->x + (src+dim)->x\

#define sum3(x) \
sum3_##x = (src-dim+1)->x + (src+1)->x + (src+dim+1)->x\

#define sum1_update(x) \
sum1_##x = (src-dim+1)->x + (src+1)->x + (src+dim+1)->x\

#define sum2_update(x) \
sum2_##x = (src-dim+1)->x + (src+1)->x + (src+dim+1)->x\

#define sum3_update(x) \
sum3_##x = (src-dim+1)->x + (src+1)->x + (src+dim+1)->x\

#define dst_update6(x) \
dst->x = (sum1_##x + sum2_##x + sum3_##x)/6\


#define dst_update9(x) \
dst->x = (sum1_##x + sum2_##x + sum3_##x)/9\

#define dst_update9_pixel(x) \
dst->x = ((src-1)->x + (src->x) + (src+1)->x + (src+dim-1)->x + (src+dim)->x + (src+dim+1)->x + (src-dim-1)->x + (src-dim)->x + (src-dim+1)->x)/9 \




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

/* #define RIDX(i, j, dim) = ((i*dim)+j)  */
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	  for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}

/* 
 * rotate - Your current working version of rotate
 * IMPORTANT: This is the version you will be graded on
 */
char rotate_descr[] = "rotate: Current working version";
void rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    int N = 16;
    for(i=0;i<dim;i+=N){
      for(j=0;j<dim;j++){
        pixel *dst2 = dst + RIDX(dim-1-j,i,dim);
        pixel *src2 = src + RIDX(i,j,dim);
        
        *(dst2++) = *src2;src2 += dim; //����16*1��Ԫ��
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;
        *(dst2++) = *src2;src2 += dim;

      }
    }
    for(;i<dim;i++){ //����ʣ����(dim��Ϊ16��������ʱ���õ�)
      for(j=0;j<dim;j++){
      dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
      }
    }
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
    add_rotate_function(&naive_rotate, naive_rotate_descr);   
    add_rotate_function(&rotate, rotate_descr);   
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
static void initialize_pixel_sum(pixel_sum *sum) 
{
    sum->red = sum->green = sum->blue = 0;
    sum->num = 0;
    return;
}

/* 
 * accumulate_sum - Accumulates field values of p in corresponding 
 * fields of sum 
 */
static void accumulate_sum(pixel_sum *sum, pixel p) 
{
    sum->red += (int) p.red;
    sum->green += (int) p.green;
    sum->blue += (int) p.blue;
    sum->num++;
    return;
}

/* 
 * assign_sum_to_pixel - Computes averaged pixel value in current_pixel 
 */
static void assign_sum_to_pixel(pixel *current_pixel, pixel_sum sum) 
{
    current_pixel->red = (unsigned short) (sum.red/sum.num);
    current_pixel->green = (unsigned short) (sum.green/sum.num);
    current_pixel->blue = (unsigned short) (sum.blue/sum.num);
    return;
}

/* 
 * avg - Returns averaged pixel value at (i,j) 
 */
static pixel avg(int dim, int i, int j, pixel *src) 
{
    int ii, jj;
    pixel_sum sum;
    pixel current_pixel;

    initialize_pixel_sum(&sum);
    for(ii = max(i-1, 0); ii <= min(i+1, dim-1); ii++) 
	for(jj = max(j-1, 0); jj <= min(j+1, dim-1); jj++) 
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
void naive_smooth(int dim, pixel *src, pixel *dst) 
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
char smooth_descr[] = "smooth: Current working version";
void smooth(int dim, pixel *src, pixel *dst) 
{
    int i,j;
    int dim_1 = dim - 1;
    int dim_3 = dim - 3;
    int sum1_red,sum2_red,sum3_red;
    int sum1_blue,sum2_blue,sum3_blue;
    int sum1_green,sum2_green,sum3_green;
    //�������Ͻ�Ԫ��
    dst->red = ((src->red) + (src+1)->red + (src+dim)->red + (src+dim+1)->red)/4;
    dst->blue = ((src->blue) + (src+1)->blue + (src+dim)->blue + (src+dim+1)->blue)/4;
    dst->green = ((src->green) + (src+1)->green + (src+dim)->green + (src+dim+1)->green)/4;
    dst+=1;
    src+=1;
    
    //������һ��
    for(i=1;i<dim_1;i++){
      row1(red);
      row1(blue);
      row1(green);
      dst+=1;
      src+=1;
    }
    
    //�������Ͻ�Ԫ��
    dst->red = ((src->red) + (src-1)->red + (src+dim)->red + (src+dim-1)->red)/4;
    dst->blue = ((src->blue) + (src-1)->blue + (src+dim)->blue + (src+dim-1)->blue)/4;
    dst->green = ((src->green) + (src-1)->green + (src+dim)->green + (src+dim-1)->green)/4;
    dst+=1;
    src+=1;
    
    //�м����򣬸����ۼ�
    for(i=1;i<dim_1;i++){
      //��һ��
      sum1_red = 0;
      sum1_green = 0;
      sum1_blue = 0;
      sum2(red);sum2(blue);sum2(green);      
      sum3(red);sum3(blue);sum3(green);
      
      dst_update6(red);dst_update6(blue);dst_update6(green);
      dst+=1;src+=1;
      
      for(j=1;j<dim_3;j+=3){
        sum1_update(red);sum1_update(blue);sum1_update(green);  //��һ�и���Ϊ������
        dst_update9(red);dst_update9(blue);dst_update9(green);
        dst+=1;src+=1;
        
        sum2_update(red);sum2_update(blue);sum2_update(green);  //�ڶ��и���Ϊ������
        dst_update9(red);dst_update9(blue);dst_update9(green);
        dst+=1;src+=1;
      
        sum3_update(red);sum3_update(blue);sum3_update(green);  //�����и���Ϊ������
        dst_update9(red);dst_update9(blue);dst_update9(green);
        dst+=1;src+=1;
        
        }
      
      
      for(;j<dim_1;j++){ //ʣ����
        dst_update9_pixel(red);
        dst_update9_pixel(blue);
        dst_update9_pixel(green);
        dst+=1;src+=1;
        }
      
      //��ǰ�����һ��
      dst->red = ((src-1)->red + (src->red) + (src-dim-1)->red + (src-dim)->red + (src+dim)->red + (src+dim-1)->red)/6;
      dst->blue = ((src-1)->blue + (src->blue) + (src-dim-1)->blue + (src-dim)->blue + (src+dim)->blue + (src+dim-1)->blue)/6;
      dst->green = ((src-1)->green + (src->green) + (src-dim-1)->green + (src-dim)->green + (src+dim)->green + (src+dim-1)->green)/6;
      dst+=1;src+=1;
      
    }
    
    //�������½�Ԫ��
    dst->red = ((src->red) + (src+1)->red + (src-dim)->red + (src-dim+1)->red)/4;
    dst->blue = ((src->blue) + (src+1)->blue + (src-dim)->blue + (src-dim+1)->blue)/4;
    dst->green = ((src->green) + (src+1)->green + (src-dim)->green + (src-dim+1)->green)/4;
    dst+=1;
    src+=1;    
    
    //�������һ��
    for(i=1;i<dim_1;i++){
      row_last(red);
      row_last(blue);
      row_last(green);
      dst+=1;
      src+=1;      
    }
    
    //�������½�Ԫ��
    dst->red = ((src->red) + (src-1)->red + (src-dim)->red + (src-dim-1)->red)/4;
    dst->blue = ((src->blue) + (src-1)->blue + (src-dim)->blue + (src-dim-1)->blue)/4;
    dst->green = ((src->green) + (src-1)->green + (src-dim)->green + (src-dim-1)->green)/4;
    dst+=1;
    src+=1;      

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

