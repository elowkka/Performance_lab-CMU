##本项目是一个函数优化项目，旨在提高处理数据时的CPE##

##rotate函数优化##

```
/* #define RIDX(i, j, dim) = ((i*dim)+j)  */
void naive_rotate(int dim, pixel *src, pixel *dst) 
{
    int i, j;
    for (i = 0; i < dim; i++)
	  for (j = 0; j < dim; j++)
	    dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
}
```

源图像存储在数组src 中，需要将源图像逆时针旋转90°得到目标图像，目标图像存储在数组dst中。 
<div align=center>
<img src="https://github.com/elowkka/Performance_lab-CMU/blob/main/png/%E5%9B%BE%E5%83%8F%E6%97%8B%E8%BD%AC%E7%A4%BA%E6%84%8F%E5%9B%BE.png">
</div>

原有的函数中，每次循环都调用了两次RIDX。一个RIDX包含一个乘法与一个加法，频繁调用RIDX会造成巨大开销。使用指针进行索引可以减少乘法的开销，处理完一个像素点后，通过一次加法即可到达下一像素点，而不用进行复杂的RIDX运算。这就需要进行循环展开，对源图像进行分块处理。 

我使用的是16次循环展开，一个指针找到对应像素，会对该列往下的16 个像素都进行处理，即每次循环对16*1个像素进行处理，减少了循环次数，也减少了条件分支判断。

在此基础上，我还加入了并行性优化。在每次循环中多加入一个指针，每个指针负责8个像素的处理，可以无需等待前一个指针完成运算再进行运算，两个指针并行加快运算速度。实际测试中发现，两路并行没有提高太多性能，四路并行甚至降低了性能，平均speedup从2.5降至2.1。这说明该程序的并行性不高，并行优化没有带来太多好处，甚至由于四路并行有更多的计算指令，效果更差了。因此最终没有选择加入并行性优化。

理论上我们还需要对读写顺序进行优化。数组存储是按行，因此读取、写入都为按行时，理论上cache命中率最高。由于写入比读取更容易成为瓶颈，因此我们让写入为按行，即每次处理16*1个像素时，让dst+=1，src+=dim。

<div align=center>
<img src="https://github.com/elowkka/Performance_lab-CMU/blob/main/png/cache%E5%91%BD%E4%B8%AD%E7%8E%87%E9%AB%98%E7%9A%84%E8%AF%BB%E5%86%99%E9%A1%BA%E5%BA%8F.png">
</div>

优化后的rotate函数如下：
```
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
```
