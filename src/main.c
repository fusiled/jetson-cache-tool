
#include <stdio.h>

#include <stdint.h>
typedef uint32_t u32;


#define gk20a 1
#define gm20b 2
#define gp106 3
#define gp10b 4
#define gv100 5
#define gv11b 6


//default value
#ifndef JTARGET
#define JTARGET gv11b
#endif

#if JTARGET == gv11b
    #include "hw/gv11b/hw_ltc_gv11b.h"
#else
    #error "dunno know what to do with target"
#endif


//do the hamming weight
u32 hweight32( u32 n){
	u32 count=0;
	while(n!=0){
		if(n & 1)
			count++;
		n = n >> 1;
	}
	return count;	
}

#define RAILGATE_ENABLE_PATH "/sys/devices/17000000.gv11b/railgate_enable"

int is_gpu_railgated(){
    FILE * fp = fopen (RAILGATE_ENABLE_PATH, "r");
    if(fp==NULL){
        printf("Can't open railgate_enable file at location %s\n", RAILGATE_ENABLE_PATH );
        return 1;
    }
    int railgate_enabled;
    fscanf (fp, "%d", &railgate_enabled); 
    fclose(fp);
    return railgate_enabled;
}

uint64_t devmem_read( const uint64_t base_target, const uint64_t offset_target);

int main(int argc, char * argv[]){

    //the gpu may be railgated, remove this if it is enabled
    if(is_gpu_railgated()){
        printf("Can't going on since the GPU is railgated, please enable it with\n\n");
        printf("\t echo 0 > %s\n\n",RAILGATE_ENABLE_PATH);
	return 1;
    }
    uint64_t gpu_base_adr = 0x17000000;
    uint64_t gpu_ofs= ltc_ltc0_lts0_tstg_cfg1_r();
    u32 ltc_ltc0_lts0_tstg_cfg1 = devmem_read(gpu_base_adr, gpu_ofs);
    //code from gm20b_determine_L2_size_bytes in ltc_gm20b.c
    u32 ways = hweight32(ltc_ltc0_lts0_tstg_cfg1_active_ways_v(ltc_ltc0_lts0_tstg_cfg1)); 
    u32 sets = 0;
    u32 active_sets_value = ltc_ltc0_lts0_tstg_cfg1_active_sets_v(ltc_ltc0_lts0_tstg_cfg1);
      if (active_sets_value == ltc_ltc0_lts0_tstg_cfg1_active_sets_all_v()) {
          sets = 64U;
      } else if (active_sets_value ==
           ltc_ltc0_lts0_tstg_cfg1_active_sets_half_v()) {
          sets = 32U;
      } else if (active_sets_value ==
           ltc_ltc0_lts0_tstg_cfg1_active_sets_quarter_v()) {
          sets = 16U;
      } else {
          printf( "Unknown constant %u for active sets",
                 (unsigned)active_sets_value);
      }
    u32 ltc_ltcs_ltss_cbc_param = devmem_read(gpu_base_adr, ltc_ltcs_ltss_cbc_param_r() );
    u32 cls = 128; //512U << ltc_ltcs_ltss_cbc_param_cache_line_size_v(ltc_ltcs_ltss_cbc_param);
    u32 lts_per_ltc = ltc_ltcs_ltss_cbc_param_slices_per_ltc_v(ltc_ltcs_ltss_cbc_param);
    u32 slice_size = ways*sets*cls;
    u32 active_ltcs =  ltc_ltcs_ltss_cbc_num_active_ltcs__v( devmem_read(gpu_base_adr, ltc_ltcs_ltss_cbc_num_active_ltcs_r())  );
    printf("cache line size (%u B ) x sets ( %u ) x ways ( %u ) = sub_partition size %u kB\n", cls, ways, sets, slice_size / 1024);
    printf("active ltcs ( %u ) x lts_per_ltc ( %u ) x slice_size ( %u ) = L2 cache size %u kB\n", active_ltcs,  lts_per_ltc, slice_size, active_ltcs*lts_per_ltc*slice_size/1024);
}
