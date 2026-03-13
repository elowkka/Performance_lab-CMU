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
        
        *(dst2++) = *src2;src2 += dim; //处理16*1个元素
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
    for(;i<dim;i++){ //处理剩余的行（dim不是16的倍数时）
      for(j=0;j<dim;j++){
      dst[RIDX(dim-1-j, i, dim)] = src[RIDX(i, j, dim)];
      }
    }
}
```

##smooth函数优化##

原smooth函数
```
void naive_smooth(int dim, pixel *src, pixel *dst) 
{
    int i, j;

    for (i = 0; i < dim; i++)
	  for (j = 0; j < dim; j++)
	    dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
}
```

原smooth 函数中，每个像素的处理都调用avg函数，avg函数中又调用了initialize_pixel_sum、min、max、accumulate_sum 等多个函数，无疑造成了巨大的开销。为了优化，我选择不使用这些函数，而是将源图像分为多个区域，每个区域依次处理。


<div align=center>
<img src="https://github.com/elowkka/Performance_lab-CMU/blob/main/png/%E5%88%86%E5%8C%BA%E5%9F%9F%E5%A4%84%E7%90%86%E7%A4%BA%E6%84%8F%E5%9B%BE.png">
</div>

每个区域的处理方法有区别，四角元素是除以4，边界为除以6，中间区域除以9。每处理一个像素，dst和src都自增1，继续处理下一个像素，这代表我们的处理方向是从左往右，换行后继续从左往右。

为了减少函数调用的开销，都直接对dst->red、dst->blue、dst->green进行逐像素赋值，第一行、最后一行与四角都是这样的直接赋值。 

中间区域的处理运用了复用的思想。正常处理中，需要对该像素本身及其周边的8个像素RGB值分别进行累加，这里的操作主要有3*9=27次访存，3*8=24 次加操作以及 3 次除操作，处理一个像素就有如此多的操作，造成了巨大的开销。

<div align=center>
<img src="https://github.com/elowkka/Performance_lab-CMU/blob/main/png/%E4%B8%AD%E9%97%B4%E5%8C%BA%E5%9F%9F%E7%B4%AF%E5%8A%A0%E5%80%BC%E7%A4%BA%E6%84%8F%E5%9B%BE.png">
</div>

注意到，两个左右相邻的像素，它们平滑操作用到的9个像素中有6个是重叠的，我们可以利用重叠的部分减少计算。例如，处理(i,j)像素时，需要分别累加左边列(i-dim-1,j-1)、(i-1,j-1)、(i+dim-1,j-1)，当前列(i-dim-1,j)、(i-1,j)、(i+dim-1,j)，右边列(i-dim+1,j+1)、(i+1,j+1)、(i+dim+1,j+1)的 RGB 值，再计算平均值。处理下一个像素(i,j+1)时，上一个像素的当前列、右边列变为当前像素的左边列与当前列，因此只需要再计算新的右边列，就可以累加计算平均值了。每次两组累加量得到复用，只需要新计算一组累加量而不是三组累加量，极大减少了开销。如图10所示，下一个像素只需要计算j+2列的累加值。

中间区域复用时，因为并不关心三列累加值的顺序，只关心三列累加值之和是否正确，所以只让左边列无用的累加值变为下一个像素的右边列累加值。因此，会出现各累加值列排序为123、231、312、123的周期性变化，因此我采用了3次循环展开，方便进行复用。 

累加值复用的关键代码如下： 
```
      dst_update6(red);dst_update6(blue);dst_update6(green);
      dst+=1;src+=1;
      
      for(j=1;j<dim_3;j+=3){
        sum1_update(red);sum1_update(blue);sum1_update(green);  //第一列更新为最新列
        dst_update9(red);dst_update9(blue);dst_update9(green);
        dst+=1;src+=1;
        
        sum2_update(red);sum2_update(blue);sum2_update(green);  //第二列更新为最新列
        dst_update9(red);dst_update9(blue);dst_update9(green);
        dst+=1;src+=1;
      
        sum3_update(red);sum3_update(blue);sum3_update(green);  //第三列更新为最新列
        dst_update9(red);dst_update9(blue);dst_update9(green);
        dst+=1;src+=1;
        
        }
```

每处理完一个像素，只要更新新的右边列累加值，就可以处理下一个像素了。并且不考虑sum1、sum2、sum3的顺序，哪个变量是上一个像素的左边列，就让其更新为新的右边列，因此每处理3个像素，又变为sum1 需要更新了。这里我就用了3次循环展开，一方面配合复用的周期性，另一方面也减少了循环次数，减少条件分支判断。


##实验结果##

##rotate函数优化结果##
<div align=center>
<img src="https://github.com/elowkka/Performance_lab-CMU/blob/main/png/%E4%BC%98%E5%8C%96%E7%BB%93%E6%9E%9C%E4%B8%80.png">
</div>

##smooth函数优化结果##
<div align=center>
<img src="https://github.com/elowkka/Performance_lab-CMU/blob/main/png/%E4%BC%98%E5%8C%96%E7%BB%93%E6%9E%9C%E4%BA%8C.png">
</div

平均speedup 达到6.3，甚至有时能达到6.6，并且每个维度的提升比较平
均，表明了优化的成功。

两个函数在优化之后均获得了较大性能提升，rotate函数平均speedup达到2.5，smooth平均speedup达到6.3。不过我也发现，在服务器的使用高峰期，测试效果会变差，smooth的平均speedup 有时会降到5.4左右，不过一般还是能达到6.0以上。 

两个函数也仍有继续优化的空间，如在并行性优化上进行更多尝试，以及进一步提高cache命中率。特别是smooth函数，循环内的操作很多，可能仍有可以避免的复杂运算，中间区域的复用也并非最优解，可能可以尝试其它的分块和循环展开方法。但总的来说，我们的性能优化是成功的。
