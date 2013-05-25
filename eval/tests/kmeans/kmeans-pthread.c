/* Copyright (c) 2007, Stanford University
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of Stanford University nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/ 

#if defined(ENABLE_DMP)
#include "dmp.h"
#endif

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include "stddefines.h"
#include "MapReduceScheduler.h"

#define DEF_NUM_POINTS 100000
//#define DEF_NUM_POINTS 100000
//#define DEF_NUM_MEANS 1000
#define DEF_NUM_MEANS 2000
#define DEF_DIM 3
#define DEF_GRID_SIZE 1000

#define false 0
#define true 1

//int modified;

char buf1[4096];

#define MAX_DIM 4

typedef struct {
   int start_idx;
   int num_pts;
  int sum[MAX_DIM]; //max size EDB FIX ME
  // EDB all new below here
  int dim;
  int num_means;
  int num_points;
  int ** means;
  int ** points;
  int * clusters;
} thread_arg;

/** dump_points()
 *  Helper function to print out the points
 */
void dump_points(int **vals, int rows, int dim)
{
   int i, j;
   
   for (i = 0; i < rows; i++) 
   {
      for (j = 0; j < dim; j++)
      {
	printf("%5d ",vals[i][j]);
      }
      printf("\n");
   }
}

/** parse_args()
 *  Parse the user arguments
 */
#if 0
void parse_args(int argc, char **argv) 
{
   int c;
   extern char *optarg;
   extern int optind;
   
   num_points = DEF_NUM_POINTS;
   num_means = DEF_NUM_MEANS;
   dim = DEF_DIM;
   grid_size = DEF_GRID_SIZE;
   
   while ((c = getopt(argc, argv, "d:c:p:s:")) != EOF) 
   {
      switch (c) {
         case 'd':
            dim = atoi(optarg);
            break;
         case 'c':
            num_means = atoi(optarg);
            break;
         case 'p':
            num_points = atoi(optarg);
            break;
         case 's':
            grid_size = atoi(optarg);
            break;
         case '?':
            printf("Usage: %s -d <vector dimension> -c <num clusters> -p <num points> -s <grid size>\n", argv[0]);
            exit(1);
      }
   }
   
   if (dim <= 0 || num_means <= 0 || num_points <= 0 || grid_size <= 0) {
      printf("Illegal argument value. All values must be numeric and greater than 0\n");
      exit(1);
   }
   
   printf("Dimension = %d\n", dim);
   printf("Number of clusters = %d\n", num_means);
   printf("Number of points = %d\n", num_points);
   printf("Size of each dimension = %d\n", grid_size);   
}
#endif

/** generate_points()
 *  Generate the points
 */
void generate_points(int **pts, int size, int dim, int grid_size) 
{   
   int i, j;
   
   for (i=0; i<size; i++) 
   {
      for (j=0; j<dim; j++) 
      {
         pts[i][j] = rand() % grid_size;
      }
   }
}

/** get_sq_dist()
 *  Get the squared distance between 2 points
 */
inline unsigned int get_sq_dist(int *v1, int *v2, int dim)
{
   int i;
   
   unsigned int sum = 0;
   for (i = 0; i < dim; i++) 
   {
      sum += ((v1[i] - v2[i]) * (v1[i] - v2[i])); 
   }
   return sum;
}

/** add_to_sum()
 *	Helper function to update the total distance sum
 */
void add_to_sum(int *sum, int *point, int dim)
{
   int i;
   
   for (i = 0; i < dim; i++)
   {
      sum[i] += point[i];   
   }   
}

/** find_clusters()
 *  Find the cluster that is most suitable for a given set of points
 */
void *find_clusters(void *arg) 
{
  int modified = false;
   thread_arg *t_arg = (thread_arg *)arg;
   int i, j;
   unsigned int min_dist, cur_dist;
   int min_idx;
   int start_idx = t_arg->start_idx;
   int end_idx = start_idx + t_arg->num_pts;
   int dim = t_arg->dim;
   int num_means = t_arg->num_means;
   int num_points = t_arg->num_points;
   int ** means = t_arg->means;
   int ** points = t_arg->points;
   int * clusters = t_arg->clusters;

   for (i = start_idx; i < end_idx; i++) 
   {
     min_dist = get_sq_dist(points[i], means[0], dim);
      min_idx = 0; 
      for (j = 1; j < num_means; j++)
      {
	cur_dist = get_sq_dist(points[i], means[j], dim);
         if (cur_dist < min_dist) 
         {
            min_dist = cur_dist;
            min_idx = j;   
         }
      }
      
      if (clusters[i] != min_idx) 
      {
         clusters[i] = min_idx;
         modified = true;
      }
   }
   
   return (void *) modified;
}

/** calc_means()
 *  Compute the means for the various clusters
 */
void *calc_means(void *arg)
{
   int i, j, grp_size;
   int *sum;
   thread_arg *t_arg = (thread_arg *)arg;
   int start_idx = t_arg->start_idx;
   int end_idx = start_idx + t_arg->num_pts;
   int dim = t_arg->dim;
   int num_points = t_arg->num_points;
   int ** means = t_arg->means;
   int * clusters = t_arg->clusters;
   int ** points = t_arg->points;

   sum = t_arg->sum;
   
   for (i = start_idx; i < end_idx; i++) 
   {
      memset(sum, 0, dim * sizeof(int));
      grp_size = 0;
      
      for (j = 0; j < num_points; j++)
      {
         if (clusters[j] == i) 
         {
	   add_to_sum(sum, points[j], dim);
            grp_size++;
         }   
      }
      
      for (j = 0; j < dim; j++)
      {
         //dprintf("div sum = %d, grp size = %d\n", sum[j], grp_size);
         if (grp_size != 0)
         { 
            means[i][j] = sum[j] / grp_size;
         }
      }       
   }
   //   free(sum);
   return (void *)0;
}

int main(int argc, char **argv)
{
   
  int num_points; // number of vectors
  int num_means; // number of clusters
  int dim;       // Dimension of each vector
  int grid_size; // size of each dimension of vector space
   int num_procs, curr_point;
   int i;
   pthread_t pid[256];
   pthread_attr_t attr;
   int num_per_thread, excess; 
  
   num_points = DEF_NUM_POINTS;
   num_means = DEF_NUM_MEANS;
   dim = DEF_DIM;
   grid_size = DEF_GRID_SIZE;
   
   {
     int c;
     extern char *optarg;
     extern int optind;
     
     while ((c = getopt(argc, argv, "d:c:p:s:")) != EOF) 
       {
	 switch (c) {
         case 'd':
	   dim = atoi(optarg);
	   break;
         case 'c':
	   num_means = atoi(optarg);
	   break;
         case 'p':
	   num_points = atoi(optarg);
	   break;
         case 's':
	   grid_size = atoi(optarg);
	   break;
         case '?':
	   printf("Usage: %s -d <vector dimension> -c <num clusters> -p <num points> -s <grid size>\n", argv[0]);
	   exit(1);
	 }
       }
   
     if (dim <= 0 || num_means <= 0 || num_points <= 0 || grid_size <= 0) {
       printf("Illegal argument value. All values must be numeric and greater than 0\n");
       exit(1);
     }
   }

   printf("Dimension = %d\n", dim);
   printf("Number of clusters = %d\n", num_means);
   printf("Number of points = %d\n", num_points);
   printf("Size of each dimension = %d\n", grid_size);   
   
   int ** points = (int **)malloc(sizeof(int *) * num_points);
   for (i=0; i<num_points; i++) 
   {
      points[i] = (int *)malloc(sizeof(int) * dim);
   }
   
   dprintf("Generating points\n");
   generate_points(points, num_points, dim, grid_size);
   
   int **means;

   means = (int **)malloc(sizeof(int *) * num_means);
   for (i=0; i<num_means; i++) 
   {
   //   means[i] = (int *)malloc(sizeof(int) * dim);
      means[i] = (int *)malloc(128);
   }
   dprintf("Generating means\n");
   generate_points(means, num_means, dim, grid_size);
 
   int * clusters = (int *)malloc(sizeof(int) * num_points);
   memset(clusters, -1, sizeof(int) * num_points);
   
   
   pthread_attr_init(&attr);
   pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
   CHECK_ERROR((num_procs = sysconf(_SC_NPROCESSORS_ONLN)) <= 0); // FIX ME
   //CHECK_ERROR((num_procs = 4 * sysconf(_SC_NPROCESSORS_ONLN)) <= 0); // FIX ME
      
   //   CHECK_ERROR( (pid = (pthread_t *)malloc(sizeof(pthread_t) * num_procs)) == NULL);


   int modified = true; 
   
   printf("Starting iterative algorithm!!!!!!\n");
   
   /* Create the threads to process the distances between the various
   points and repeat until modified is no longer valid */
   int num_threads;   
   thread_arg arg[256];
   while (modified) 
   {
      num_per_thread = num_points / num_procs;
      excess = num_points % num_procs;
      modified = false;
      dprintf(".");
      curr_point = 0;
      num_threads = 0;
      
      while (curr_point < num_points) {
	//	CHECK_ERROR((arg = (thread_arg *)malloc(sizeof(thread_arg))) == NULL);
         arg[num_threads].start_idx = curr_point;
         arg[num_threads].num_pts = num_per_thread;
	     arg[num_threads].dim = dim;
	     arg[num_threads].num_means = num_means;
	     arg[num_threads].num_points = num_points;
	     arg[num_threads].means = means;
	     arg[num_threads].points = points;
	     arg[num_threads].clusters = clusters;

         if (excess > 0) {
            arg[num_threads].num_pts++;
            excess--;            
         }
         curr_point += arg[num_threads].num_pts;
	     num_threads++;
      }

//	  printf("in this run, num_threads is %d, num_per_thread is %d\n", num_threads, num_per_thread);
      
	  for (i = 0; i < num_threads; i++) {
         CHECK_ERROR((pthread_create(&(pid[i]), &attr, find_clusters,
                                                   (void *)(&arg[i]))) != 0);
	 // EDB - with hierarchical commit we would not have had to
	 // "localize" num_threads.
      }
      
      assert (num_threads == num_procs);
      for (i = 0; i < num_threads; i++) {
	     int m;
	     pthread_join(pid[i], (void *) &m);
	     modified |= m;
      }
      
      num_per_thread = num_means / num_procs;
      excess = num_means % num_procs;
      curr_point = 0;
      num_threads = 0;

      assert (dim <= MAX_DIM);
	//  printf("in this run again, num_threads is %d, num_per_thread is %d\n", num_threads, num_per_thread);

      while (curr_point < num_means) {
	//	CHECK_ERROR((arg = (thread_arg *)malloc(sizeof(thread_arg))) == NULL);
	    arg[num_threads].start_idx = curr_point;
	//	arg[num_threads].sum = (int *)malloc(dim * sizeof(int));
	    arg[num_threads].num_pts = num_per_thread;
	    if (excess > 0) {
	        arg[num_threads].num_pts++;
	        excess--;            
	    }
	    curr_point += arg[num_threads].num_pts;
	    num_threads++;
      }

      for (i = 0; i < num_threads; i++) {
         CHECK_ERROR((pthread_create(&(pid[i]), &attr, calc_means,
                                                   (void *)(&arg[i]))) != 0);
      }

//      printf ("num threads = %d\n", num_threads);
//      printf ("num procs = %d\n", num_procs);

      assert (num_threads == num_procs);
      for (i = 0; i < num_threads; i++) {
		pthread_join(pid[i], NULL);
	 //	 free (arg[i].sum);
      }
      
   }
   
      
   dprintf("\n\nFinal means:\n");
   //dump_points(means, num_means, dim);

   for (i = 0; i < num_points; i++) 
      free(points[i]);
   free(points);
   
   for (i = 0; i < num_means; i++) 
   {
      free(means[i]);
   }
   free(means);
   free(clusters);

   return 0;
}
