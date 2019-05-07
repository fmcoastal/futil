#include <stdint.h>
#include <stdio.h>

#include "cvmx.h"
#include "cvmx-interrupt.h"
#include "cvmx-sysinfo.h"
#include "cvmx-pow.h"
#include "cvmx-bootmem.h"
#include "cvmx-coremask.h"
#include "cvmx-ciu2-defs.h"


volatile uint64_t current_cycle_count;

cvmx_wqe_t *g_timer_event;

static void timer_handler(int irq, uint64_t regs[32], void *arg)
{

//	printf ("Elapsed time to get the interrupt (%ld\n", 
//	    cvmx_clock_get_count(CVMX_CLOCK_SCLK) - current_cycle_count);//Ignore the first print on console
//	current_cycle_count=cvmx_clock_get_count(CVMX_CLOCK_SCLK);

// Record the time at which we handled the interrupt
     cvmx_ciu2_en_iox_int_mio_w1c_t  ack;
    *(uint64_t *)(g_timer_event->packet_data+sizeof(uint64_t)) = cvmx_clock_get_count(CVMX_CLOCK_SCLK);

// Send the work event
    cvmx_pow_work_submit (g_timer_event, g_timer_event->word1.tag, g_timer_event->word1.tag_type, 
	g_timer_event->word1.cn68xx.qos, g_timer_event->word1.cn68xx.grp);

// Ack the interrupt so it can fire again
      ack.u64 = 0;
      ack.s.timer = 1;
   cvmx_write_csr (CVMX_CIU2_RAW_PPX_IP2_MIO(0),ack.u64);


}

#define NOT_CORE_0 
#define MY_CORE 1
#define TIMER_WQE_GROUP  0x04
int InitPIT(void)
{
    cvmx_sysinfo_t *info;
#ifdef NOT_CORE_0
    int corenum;
#endif
// Need to know what the CPU clock frequency is

    info = cvmx_sysinfo_get();
    if (!info) {
        printf ("Sysinfo get failed\n");
        return -1;
    }
#ifdef NOT_CORE_0
     corenum = cvmx_get_core_num();
     if(corenum != MY_CORE)
     {
         return 0;
     } 
#endif

// Allocate and initialize the WQE buffer we will be using for the timer event

    if (cvmx_coremask_first_core (info->core_mask)) {
        g_timer_event = cvmx_bootmem_alloc(128, 128);
        if (!g_timer_event) {
            printf ("Failed to allocate WQE buffer\n");
            return -2;
        }
        memset (g_timer_event, 0, 128);
        cvmx_wqe_set_tag(g_timer_event, 0xDEAD);
        cvmx_wqe_set_grp(g_timer_event, TIMER_WQE_GROUP);
        cvmx_wqe_set_tt (g_timer_event, CVMX_POW_TAG_TYPE_ORDERED);
        cvmx_wqe_set_qos(g_timer_event, 0x7);
        *(uint64_t *)(g_timer_event->packet_data) =  (uint64_t)0xF00DCAFE;
/*
 *  // always enable core 0 to process group 4  -  a periodic Timer
 *      cvmx_write_csr( CVMX_SSO_PPX_GRP_MSK(0),( (0x01<<4) | cvmx_read_csr(CVMX_SSO_PPX_GRP_MSK(0)))); 
 * */
#ifdef NOT_CORE_0
     cvmx_write_csr( CVMX_SSO_PPX_GRP_MSK(corenum),( (0x01 << TIMER_WQE_GROUP) | cvmx_read_csr(CVMX_SSO_PPX_GRP_MSK(corenum)))); 
#endif

// Register our handler and unmask the interrupt
//    note: the code uses IP2 & Core 0
//              CIU2_RAW_PP0_IP2_MIO[TIME0]  R/W1C
//              CIU2_EN_PP0_IP2_MIO[TIME0]   R/W
//              CIU2_SRC_PP0_IP2_MIO[TIME0]  Ro

        cvmx_interrupt_register (CVMX_IRQ_TIMER0, timer_handler, (void *)NULL);
        cvmx_interrupt_unmask_irq (CVMX_IRQ_TIMER0);

// Enable a periodic interrupt
//   CIU has 4 General Timers.  We will use Timer 0 in Periodic mode.
//   THe timer clocks at the co-processor frequency.  See HRM
        cvmx_ciu_timx_t tim;
        tim.u64 = 0;
        tim.s.len = cvmx_clock_get_rate(CVMX_CLOCK_SCLK); // You can change this according to your usage
        cvmx_write_csr (CVMX_CIU_TIMX(0), tim.u64);

#if 0
    printf(" CLOCK RATE:                  0x%016llx  \n",CAST64(cvmx_clock_get_rate(CVMX_CLOCK_SCLK)));
    printf(" CVMX_CUI_TIMX(0):            0x%016llx  \n",CAST64(CVMX_CIU_TIMX(0)));
    printf(" CVMX_CIU2_RAW_PPX_IP2_MIO(0):0x%016llx  \n",CAST64(CVMX_CIU2_RAW_PPX_IP2_MIO(0)));
    printf(" CVMX_IRQ_TIMER0:             0x%016llx  \n",CAST64(CVMX_IRQ_TIMER0));
#endif


    } /* end First Core check */

    return 0;

}


#ifdef DEBUG



int main()
{
    cvmx_wqe_t *work;

// Get ready to use SE

    cvmx_user_app_init();

    InitPIT();


// Gather the work and do something

    while (1) {

        work = cvmx_pow_work_request_sync (CVMX_POW_NO_WAIT);
        if (!work) {
            continue;
        }
        printf ("Elapsed time from add to get %ld\n",
            cvmx_clock_get_count(CVMX_CLOCK_SCLK) - *(uint64_t *)(work->packet_data));
    }

    return 0;
}
#endif



#if 0

int main()
{
    cvmx_sysinfo_t *info;
    cvmx_wqe_t *work;

// Get ready to use SE

    cvmx_user_app_init();

// Need to know what the CPU clock frequency is

    info = cvmx_sysinfo_get();
    if (!info) {
	printf ("Sysinfo get failed\n");
	return 1;
    }

// Allocate and initialize the WQE buffer we will be using for the timer event

    if (cvmx_coremask_first_core (info->core_mask)) {
	g_timer_event = cvmx_bootmem_alloc(128, 128);
	if (!g_timer_event) {
	    printf ("Failed to allocate WQE buffer\n");
	    return 1;
	}
	memset (g_timer_event, 0, 128);
	cvmx_wqe_set_tag(g_timer_event, 0xDEAD);
	cvmx_wqe_set_grp(g_timer_event, 0x1);
	cvmx_wqe_set_tt (g_timer_event, CVMX_POW_TAG_TYPE_ORDERED);
	cvmx_wqe_set_qos(g_timer_event, 0x7);
        *(uint64_t *)((g_timer_event->packet_data) = 0xFOODCAFE;

// Register our handler and unmask the interrupt

	cvmx_interrupt_register (CVMX_IRQ_TIMER0, timer_handler, (void *)NULL);
	cvmx_interrupt_unmask_irq (CVMX_IRQ_TIMER0);

// Enable a periodic interrupt
	cvmx_ciu_timx_t tim;
	tim.u64 = 0;
	tim.s.len = cvmx_clock_get_rate(CVMX_CLOCK_SCLK); // You can change this according to your usage
	cvmx_write_csr (CVMX_CIU_TIMX(0), tim.u64);
    }
    

// Gather the work and do something

    while (1) {

	work = cvmx_pow_work_request_sync (CVMX_POW_NO_WAIT);
	if (!work) {
	    continue;
	}
	printf ("Elapsed time from add to get %ld\n", 
	    cvmx_clock_get_count(CVMX_CLOCK_SCLK) - *(uint64_t *)(work->packet_data));
    }

    return 0;
}

#endif
