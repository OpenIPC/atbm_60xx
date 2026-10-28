
#include <assert.h>
#include <linux/fs.h>
//#include <linux/kernel.h>
//#include <linux/module.h>
#include <linux/string.h>
#include "os/queue.h"


#include "atbm_hal.h"
#include "services/gap/ble_svc_gap.h"
#include "services/dis/ble_svc_dis.h"
#include "services/gatt/ble_svc_gatt.h"

#include "host/ble_gatt.h"
#include "host/ble_att.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "nimble/ble_hci_trans.h"
#include "atbm_os_api.h"
#include "ble_master_demo.h"
#include "ble_smart_cfg.h"
#include "peer.h"
#include <unistd.h> 
pAtbm_thread_t ble_get_gatt_thread = NULL;
static  char scan_ble_name[31];
extern bool atbm_ble_is_quit;
pAtbm_thread_t ble_subscribe_thread = NULL;
uint16_t ble_master_conn_handle=0;
uint8_t atbm_ble_master_status=0;

#if 1
#define BLE_SVC_ALERT_UUID              0x1805
#define BLE_CHR_UUID  0x2829

extern void atbm_print_conn_desc(const struct ble_gap_conn_desc *desc);
static const struct {
    uint8_t bit;
    const char *name;
} chr_props[] = {
    { BLE_GATT_CHR_PROP_BROADCAST, "BROADCAST" },
    { BLE_GATT_CHR_PROP_READ, "READ" },
    { BLE_GATT_CHR_PROP_WRITE_NO_RSP, "WRITE_NO_RSP" },
    { BLE_GATT_CHR_PROP_WRITE, "WRITE" },
    { BLE_GATT_CHR_PROP_NOTIFY, "NOTIFY" },
    { BLE_GATT_CHR_PROP_INDICATE, "INDICATE" },
    { BLE_GATT_CHR_PROP_AUTH_SIGN_WRITE, "AUTH_SIGN_WRITE" },
    { BLE_GATT_CHR_PROP_EXTENDED, "EXTENDED" },
};
static const char *chr_props_to_str(uint8_t properties) {
    static char buf[128];
    char *p = buf;
    int i;

    *p = '\0';
    for (i = 0; i < sizeof(chr_props) / sizeof(chr_props[0]); i++) {
        if (properties & chr_props[i].bit) {
            if (p != buf) {
                p += sprintf(p, "|");
            }
            p += sprintf(p, "%s", chr_props[i].name);
            properties &= ~chr_props[i].bit;
        }
    }

    // 处理未知标志位
    if (properties != 0) {
        if (p != buf) {
            p += sprintf(p, "|");
        }
        p += sprintf(p, "0x%02x", properties);
    }

    return buf;
}

void print_service(struct peer_svc *svc) {
    char uuid_str[BLE_UUID_STR_LEN];
    ble_uuid_to_str(&svc->svc.uuid, uuid_str);
    printf("\n[Service] UUID: %s, Handles: %d-%d\n",
           uuid_str, svc->svc.start_handle, svc->svc.end_handle);
}

void print_characteristic(struct peer_chr *chr) {
    char uuid_str[BLE_UUID_STR_LEN];
    ble_uuid_to_str(&chr->chr.uuid, uuid_str);
    printf("  [Characteristic] UUID: %s, Handle: %d, Properties: %s (Props: 0x%02x)\n",
           uuid_str, chr->chr.def_handle,  chr_props_to_str(chr->chr.properties),chr->chr.properties);
}

void print_descriptor(struct peer_dsc *dsc) {
    char uuid_str[BLE_UUID_STR_LEN];
    ble_uuid_to_str(&dsc->dsc.uuid, uuid_str);
    printf("    [Descriptor] UUID: %s, Handle: %d\n",
           uuid_str, dsc->dsc.handle);
}

void print_all_attributes(struct peer *peer) {
    struct peer_svc *svc;
    SLIST_FOREACH(svc, &peer->svcs, next) {
        print_service(svc);

        struct peer_chr *chr;
        SLIST_FOREACH(chr, &svc->chrs, next) {
            print_characteristic(chr);

            struct peer_dsc *dsc;
            SLIST_FOREACH(dsc, &chr->dscs, next) {
                print_descriptor(dsc);
            }
        }
    }
}


/**
 * Application callback.  Called when the attempt to subscribe to notifications
 * for the ANS Unread Alert Status characteristic has completed.
 */
static int
ble_on_subscribe(uint16_t conn_handle,
                     const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr,
                     void *arg)
{
    iot_printf( "Subscribe complete; status=%d conn_handle=%d "
                      "attr_handle=%d\n",
                error->status, conn_handle, attr->handle);

    return 0;
}

/**
 * Application callback.  Called when the write to the ANS Alert Notification
 * Control Point characteristic has completed.
 */
static int
ble_on_write(uint16_t conn_handle,
                 const struct ble_gatt_error *error,
                 struct ble_gatt_attr *attr,
                 void *arg)
{
    iot_printf( "Write complete; status=%d conn_handle=%d attr_handle=%d\n",
                error->status, conn_handle, attr->handle);

    return 0;
}


/**
 * Application callback.  Called when the read of the ANS Supported New Alert
 * Category characteristic has completed.
 */
static int
ble_on_read(uint16_t conn_handle,
                const struct ble_gatt_error *error,
                struct ble_gatt_attr *attr,
                void *arg)
{
    iot_printf("Read complete; status=%d conn_handle=%d", error->status,
                conn_handle);
    if (error->status == 0) {
        iot_printf(" attr_handle=%d value=", attr->handle);
       // print_mbuf(attr->om);
    }
    iot_printf("\n");

    return 0;
}

static void
ble_read_write_subscribe(const struct peer *peer)
{
    const struct peer_chr *chr;
    const struct peer_dsc *dsc;
    uint8_t value[2];
    int rc;
	iot_printf("ble_read_write_subscribe\n");
    /* Read the supported-new-alert-category characteristic. */
    chr = peer_chr_find_uuid(peer,
                             BLE_UUID16_DECLARE(BLE_SVC_ALERT_UUID),
                             BLE_UUID16_DECLARE(BLE_CHR_UUID));
    if (chr == NULL) {
        iot_printf("Error: Peer doesn't support the Supported New "
                           "Alert Category characteristic\n");
        goto err;
    }

    rc = ble_gattc_read(peer->conn_handle, chr->chr.val_handle,
                        ble_on_read, NULL);
    if (rc != 0) {
        iot_printf("Error: Failed to read characteristic; rc=%d\n",
                    rc);
        goto err;
    }

    /* Write two bytes (99, 100) to the alert-notification-control-point
     * characteristic.
     */
    chr = peer_chr_find_uuid(peer,
                             BLE_UUID16_DECLARE(BLE_SVC_ALERT_UUID),
                             BLE_UUID16_DECLARE(BLE_CHR_UUID));
    if (chr == NULL) {
        iot_printf("Error: Peer doesn't support the Alert "
                           "Notification Control Point characteristic\n");
        goto err;
    }

    value[0] = 99;
    value[1] = 100;
    rc = ble_gattc_write_flat(peer->conn_handle, chr->chr.val_handle,
                              value, sizeof value, ble_on_write, NULL);
    if (rc != 0) {
        iot_printf( "Error: Failed to write characteristic; rc=%d\n",
                    rc);
    }

    /* Subscribe to notifications for the Unread Alert Status characteristic.
     * A central enables notifications by writing two bytes (1, 0) to the
     * characteristic's client-characteristic-configuration-descriptor (CCCD).
     */
    dsc = peer_dsc_find_uuid(peer,
                             BLE_UUID16_DECLARE(BLE_SVC_ALERT_UUID),
                             BLE_UUID16_DECLARE(BLE_CHR_UUID),
                             BLE_UUID16_DECLARE(BLE_GATT_DSC_CLT_CFG_UUID16));
    if (dsc == NULL) {
        iot_printf( "Error: Peer lacks a CCCD for the Unread Alert "
                           "Status characteristic\n");
        goto err;
    }

    value[0] = 1;
    value[1] = 0;
    rc = ble_gattc_write_flat(peer->conn_handle, dsc->dsc.handle,
                              value, sizeof value, ble_on_subscribe, NULL);
    if (rc != 0) {
        iot_printf("Error: Failed to subscribe to characteristic; "
                           "rc=%d\n", rc);
        goto err;
    }

    return;

err:
    /* Terminate the connection. */
    ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
}
#endif

static void
ble_on_disc_complete(const struct peer *peer, int status, void *arg)
{

    if (status != 0) {
        /* Service discovery failed.  Terminate the connection. */
        iot_printf("Error: Service discovery failed; status=%d "
                           "conn_handle=%d\n", status, peer->conn_handle);
        ble_gap_terminate(peer->conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        return;
    }

    /* Service discovery has completed successfully.  Now we have a complete
     * list of services, characteristics, and descriptors that the peer
     * supports.
     */
    iot_printf("Service discovery complete; status=%d "
                       "conn_handle=%d\n", status, peer->conn_handle);

    /* Now perform three concurrent GATT procedures against the peer: read,
     * write, and subscribe to notifications.
     */
   // ble_read_write_subscribe(peer);
   print_all_attributes(peer);
}

extern void ble_print_adv_fields(const struct ble_hs_adv_fields *fields);
extern void atbm_print_conn_desc(const struct ble_gap_conn_desc *desc);


//struct ble_host_dis_gap_gatt ble_conn_m[MYNEWT_VAL_BLE_MAX_CONNECTIONS];

uint32_t ble_wait_gatt_notify=0;
uint32_t ble_wait_gatt_indicate=0;
SRAM_CODE static void
ble_print_error(char *msg, uint16_t conn_handle,
                   const  struct ble_gatt_error *error)
{
    if (msg == NULL) {
        msg = "ERROR";
    }

    iot_printf("%s: conn_handle=%d status=%d att_handle=%d\n",
                   msg, conn_handle, error->status, error->att_handle);
}


	
static int ble_atbm_notify_on_subscribe(uint16_t conn_handle,
                     const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr,
                     void *arg)
{
    iot_printf("\n\n\n\n\nnotify_Subscribe complete; status=%d conn_handle=%d "
                      "attr_handle=%d\n\n\n\n\n",
                error->status, conn_handle, attr->handle);
	
	ble_wait_gatt_notify = 0;
    return 0;
}
static int ble_atbm_indicate_on_subscribe(uint16_t conn_handle,
                     const struct ble_gatt_error *error,
                     struct ble_gatt_attr *attr,
                     void *arg)
{
    iot_printf("\n \n\nindicate_Subscribe complete; status=%d conn_handle=%d "
                      "attr_handle=%d\n\n\n\n\n",
                error->status, conn_handle, attr->handle);
	
	ble_wait_gatt_indicate = 0;
    return 0;
}

int32_t gatt_client_notify_on(uint32_t conn_handle, uint16_t desc_handle, bool on)
{
	int rc;
	uint8_t value[2];
       ble_wait_gatt_notify=0;
	if((conn_handle > 4) || (conn_handle == 0)){
		iot_printf("ble notify callback fd error(%d) must in range 1 ~ 4\n", conn_handle);
		return -1;
	}

	if(on){
		value[0] = 1;
		value[1] = 0;
	}
	else{
		value[0] = 0;
		value[1] = 0;		
	}
	ble_wait_gatt_notify = 1;
	rc = ble_gattc_write_flat(conn_handle, desc_handle, value, 2, ble_atbm_notify_on_subscribe, NULL);
    if (rc != 0) {
        iot_printf("error notify on; rc=%d\n", rc);
		return rc;
    }
    
	while(ble_wait_gatt_notify == 1){
		usleep(10000);
	}
	
	return 0;
}
int32_t gatt_client_indicate_on(uint32_t conn_handle, uint16_t desc_handle, bool on)
{
	int rc;
	uint8_t value[2];
      ble_wait_gatt_indicate=0;
	if((conn_handle > 4) || (conn_handle == 0)){
		iot_printf("ble notify callback fd error(%d) must in range 1 ~ 4\n", conn_handle);
		return -1;
	}

	if(on){
		value[0] = 2;
		value[1] = 0;
		
	}
	else{
		value[0] = 0;
		value[1] = 0;		
	}
	ble_wait_gatt_indicate = 1;
	rc = ble_gattc_write_flat(conn_handle, desc_handle, value, 2 ,ble_atbm_indicate_on_subscribe, NULL);
    if (rc != 0) {
        iot_printf("error notify on; rc=%d\n", rc);
		return rc;
    }
    
	while(ble_wait_gatt_indicate == 1){
	
		usleep(10000);
	}
	
	return 0;
}
SRAM_CODE static int ble_subscribe_test(void* argc)
{
	int rc;
	iot_printf("ble_subscribe_test \n");
       rc=gatt_client_notify_on(ble_master_conn_handle,0x34,1);
	if(rc)
		iot_printf("gatt_client_notify_on erorr=%d\n",rc);
      
	rc=gatt_client_indicate_on(ble_master_conn_handle,0x38,1); 
	if(rc)
		iot_printf("gatt_client_indicate_on erorr=%d\n",rc);	  
	 
	atbm_stopThread(ble_subscribe_thread);
	ble_subscribe_thread = NULL;
}
SRAM_CODE static int ble_get_gatt_task(void* argc)
{
	int rc;
	if(atbm_ble_master_status==ATBM_BLE_STATUS_CONNECT)
	{
		usleep(100*1000);
		
	}
	atbm_stopThread(ble_get_gatt_thread);
	ble_get_gatt_thread=NULL;
}

//extern SRAM_CODE  int btshell_gap_event(struct ble_gap_event *event, void *arg);

int ble_master_gap_event(struct ble_gap_event *event, void *arg)
{
	struct ble_gap_conn_desc desc;
	struct ble_sm_io pkey;
	struct ble_gap_conn_params phy_1M_params = {0};
	int32_t duration_ms;
	int own_addr_type;
	ble_addr_t peer_addr;
	/* Connection handle */
	//uint16_t conn_handle=1;
	struct ble_gap_upd_params params;
	params.itvl_min=30;////Range: 0x0006 to 0x0C80 Time = N * 1.25 msec	      1*1.25=7.5ms
	params.itvl_max=48;
	params.latency=0;
	params.supervision_timeout=300;//????3?¡§o?¨¤¡§o?¨¤??Supervision timeout for the LE Link.Range: 0x000A to 0x0C80Time = connTimeout* 10 msecTime Range: 100 msec to 32 seconds
	int conn_idx=0;
	int rc=0;	
	uint8_t i=0;
	uint8_t test_data[254];
	 int svc_start_handle=0;
	struct ble_hs_adv_fields fields;
	uint8_t ble_name[30];
	iot_printf("m_event=%d\n",event->type);
	switch (event->type) {
	case BLE_GAP_EVENT_CONNECT:
		iot_printf("connection %s; status=%d ",
		               event->connect.status == 0 ? "established" : "failed",
		               event->connect.status);
		if (event->connect.status == 0) 
		{
			atbm_ble_master_status=ATBM_BLE_STATUS_CONNECT;
			ble_att_set_preferred_mtu(244);
		    rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
		    assert(rc == 0);
			ble_master_conn_handle = event->connect.conn_handle;
		    atbm_print_conn_desc(&desc);
		    //ble_conn_add(&desc);
		    iot_printf("connection, handle=%d\n",event->connect.conn_handle);
			/* Initialize data structures to track connected peers. */
			//rc = peer_init(1, 64, 64, 64);
		//	assert(rc == 0);
            /* Remember peer. */
	        peer_add(event->connect.conn_handle);

			rc = peer_disc_all(event->connect.conn_handle,
		                               ble_on_disc_complete, NULL);
		    if (rc != 0) {
		        iot_printf("Failed to peer_disc_all; rc=0x%x\n", rc);
		        return 0;
		    }
		 }
		else {
			iot_printf("Connection failed; resume connet\n");
			if(!atbm_ble_is_quit)
				ble_master_connect(BLE_CONN_NAME);
			/* Connection failed; resume advertising */
			
		}
		return 0;
		
	case BLE_GAP_EVENT_DISCONNECT:
		atbm_ble_status=ATBM_BLE_STATUS_IDLE;
	    iot_printf("disconnect; reason=0x%x \n", event->disconnect.reason);
	    atbm_print_conn_desc(&event->disconnect.conn);
		/* Forget about peer. */
        peer_delete(event->disconnect.conn.conn_handle);
		if(!atbm_ble_is_quit)
			ble_master_connect(BLE_CONN_NAME);
	 

	    return 0;
#if (MYNEWT_VAL_BLE_EXT_ADV)
	case BLE_GAP_EVENT_EXT_DISC:
	    btshell_decode_event_type(&event->ext_disc, arg);
	    return 0;
#endif
	case BLE_GAP_EVENT_DISC:
		atbm_ble_master_status=ATBM_BLE_STATUS_SCANING;
		#if 1
			//if (event->disc.event_type == BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
				//return 0;
			//}
			ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data);
			ble_name[fields.name_len]=0;
			if(fields.name && fields.name_len){
				memcpy(ble_name, fields.name, fields.name_len);
				
				if(0 == memcmp(scan_ble_name, fields.name, fields.name_len))
				{
					ble_gap_disc_cancel();
					
				//if(0 == memcmp(ble_scan_show_name, fields.name, fields.name_len))
					iot_printf("\n********************ble scan adv data*****************************\n");
					iot_printf("ble_name:%s\n",ble_name);
				
					iot_printf("received advertisement; event_type=%d rssi=%d "
								   "addr_type=%d addr=", event->disc.event_type,
								   event->disc.rssi, event->disc.addr.type);
					atbm_print_addr(event->disc.addr.val);
					iot_printf(" fields:\n");
					ble_print_adv_fields(&fields);
					iot_printf("\n");
					memcpy(&peer_addr.val,event->disc.addr.val,6);
					for (i = 0; i < 6; i ++) 
					{
		                printf("0x%02x ", event->disc.addr.val[i]);
					}
					iot_printf("\n");
					own_addr_type=BLE_ADDR_PUBLIC;
					peer_addr.type=event->disc.addr.type;
					//iot_printf("own_addr_type:%d\n",own_addr_type);
					duration_ms=INT32_MAX;
					phy_1M_params.itvl_min=BLE_GAP_INITIAL_CONN_ITVL_MIN;
					 phy_1M_params.itvl_max=BLE_GAP_INITIAL_CONN_ITVL_MAX;
					 phy_1M_params.scan_window=0x0010;
					 phy_1M_params.scan_itvl=0x0010;
					 phy_1M_params.latency=0;
					 phy_1M_params.supervision_timeout=0x0100;
					 phy_1M_params.min_ce_len=0x0010;
					 phy_1M_params.max_ce_len=0x0300;
				
					rc=ble_gap_connect(own_addr_type, &peer_addr, duration_ms, &phy_1M_params,
                       		 ble_master_gap_event, NULL);
					if(rc)
					 iot_printf("error connecting; rc=%d\n", rc);
				}
				iot_printf("\n********************end*****************************\n");
			}	
	        /*
	         * There is no adv data to print in case of connectable
	         * directed advertising
	         */
	        if (event->disc.event_type == BLE_HCI_ADV_RPT_EVTYPE_DIR_IND) {
	                iot_printf("\nConnectable directed advertising event\n");
	                return 0;
	        }
			
			//ble_parse_smt_adv_data(&ble_rx_adv_data);
	        //btshell_decode_adv_data(event->disc.data, event->disc.length_data, arg);
	#endif
	    return 0;

	case BLE_GAP_EVENT_CONN_UPDATE:
	    iot_printf("connection updated; status=%d ",
	                   event->conn_update.status);
	    rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
	    assert(rc == 0);
	    atbm_print_conn_desc(&desc);
		iot_printf(" conn_itvl=%d conn_latency=%d supervision_timeout=%d "
					   "encrypted=%d authenticated=%d bonded=%d\n",
					   desc.conn_itvl, desc.conn_latency,
					   desc.supervision_timeout,
					   desc.sec_state.encrypted,
					   desc.sec_state.authenticated,
					   desc.sec_state.bonded);
		
	    return 0;

	case BLE_GAP_EVENT_CONN_UPDATE_REQ:
	    iot_printf("connection update request\n");
	    *event->conn_update_req.self_params =
	        *event->conn_update_req.peer_params;
	    return 0;

	case BLE_GAP_EVENT_PASSKEY_ACTION:
	    iot_printf("passkey action event; action=%d",
	                   event->passkey.params.action);
	    if (event->passkey.params.action == BLE_SM_IOACT_NUMCMP) {
	        iot_printf(" numcmp=%lu",
	                       (unsigned long)event->passkey.params.numcmp);
	    }else{
	        if(event->passkey.params.action == BLE_SM_IOACT_INPUT){
	            pkey.action = BLE_SM_IOACT_INPUT;
				//pkey.passkey =123456;//PTS¡§|?€¡§¡§|?¡§¡§??¡ã??-?23456??¡ê¡è¡§|€?¡§¡§??¡§|a?¡§¡§?£¤??€?
				//ble_sm_inject_io(event->passkey.conn_handle, &pkey);
			}

		}
	    iot_printf("\n");
	    return 0;


	case BLE_GAP_EVENT_DISC_COMPLETE:
	    atbm_ble_status=ATBM_BLE_STATUS_IDLE;
	    iot_printf("discovery complete; reason=%d\n",
	                   event->disc_complete.reason);
	    return 0;

	case BLE_GAP_EVENT_ADV_COMPLETE:
#if (MYNEWT_VAL_BLE_EXT_ADV)
	    iot_printf("advertise complete; reason=%d, instance=%u, handle=%d\n",
	                   event->adv_complete.reason, event->adv_complete.instance,
	                   event->adv_complete.conn_handle);

	    ext_adv_restart[event->adv_complete.instance].conn_handle =
	        event->adv_complete.conn_handle;
#else
	    iot_printf("advertise complete; reason=%d\n",
	                   event->adv_complete.reason);
#endif
	    return 0;

	case BLE_GAP_EVENT_ENC_CHANGE:
	    iot_printf("encryption change event; status=%d ",
	                   event->enc_change.status);
	    rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
	    assert(rc == 0);
	    //atbm_print_conn_desc(&desc);
	    return 0;

	case BLE_GAP_EVENT_NOTIFY_RX:
	    iot_printf("notification rx event; attr_handle=%d indication=%d "
	                   "len=%d data=",
	                   event->notify_rx.attr_handle,
	                   event->notify_rx.indication,
	                   OS_MBUF_PKTLEN(event->notify_rx.om));

	      //print_mbuf(event->notify_rx.om);
	      iot_printf("\n");
		 
		 
		return 0;

	case BLE_GAP_EVENT_NOTIFY_TX:
	    iot_printf("notification tx event; status=%d attr_handle=%d "
	                   "indication=%d\n",
	                   event->notify_tx.status,
	                   event->notify_tx.attr_handle,
	                   event->notify_tx.indication);
	    return 0;

	case BLE_GAP_EVENT_SUBSCRIBE:
	    iot_printf("subscribe event; conn_handle=%d attr_handle=%d "
	                   "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
	                   event->subscribe.conn_handle,
	                   event->subscribe.attr_handle,
	                   event->subscribe.reason,
	                   event->subscribe.prev_notify,
	                   event->subscribe.cur_notify,
	                   event->subscribe.prev_indicate,
	                   event->subscribe.cur_indicate);
		#if 1
	    if(ble_svc_gatt_changed_val_handle==event->subscribe.attr_handle)
	    {
	    		//log ?aindicate¡§¡§?¡ì?Tsubscribe event; conn_handle=1 attr_handle=10 reason=1 prevn=0 curn=0 previ=0 curi=1
	    		//log1?indicate¡§¡§?¡ì?Tsubscribe event; conn_handle=1 attr_handle=10 reason=1 prevn=0 curn=0 previ=1 curi=0
		    if((event->subscribe.prev_indicate==0)&&(event->subscribe.cur_indicate==1))
		    {	   
			    char* ble_indicate ="send indicate";
			     iot_printf("ble_svc_gatt_changed_val_handle=%d\n",
			                   ble_svc_gatt_changed_val_handle);
			    rc=gatt_svr_chr_indicate(ble_conn_handle, ble_svc_gatt_changed_val_handle, ble_indicate, strlen(ble_indicate));
			   if(rc)
	   				iot_printf("gatt_svr_chr_indicate erorr=%d\n",rc);	
				
				
		    }
	    }
		#endif
	    return 0;

	case BLE_GAP_EVENT_MTU:
	    iot_printf("mtu update event; conn_handle=%d cid=%d mtu=%d\n",
	                   event->mtu.conn_handle,
	                   event->mtu.channel_id,
	                   event->mtu.value);
		 
		
	    return 0;

	case BLE_GAP_EVENT_IDENTITY_RESOLVED:
	    iot_printf("identity resolved ");
	    rc = ble_gap_conn_find(event->identity_resolved.conn_handle, &desc);
	    assert(rc == 0);
	    //atbm_print_conn_desc(&desc);
	    return 0;

	case BLE_GAP_EVENT_PHY_UPDATE_COMPLETE:
	    iot_printf("PHY update complete; status=%d, conn_handle=%d "
	                   " tx_phy=%d, rx_phy=%d\n",
	                   event->phy_updated.status,
	                   event->phy_updated.conn_handle,
	                   event->phy_updated.tx_phy,
	                   event->phy_updated.rx_phy);
	    return 0;

	case BLE_GAP_EVENT_REPEAT_PAIRING:
	    /* We already have a bond with the peer, but it is attempting to
	     * establish a new secure link.  This app sacrifices security for
	     * convenience: just throw away the old bond and accept the new link.
	     */

	    /* Delete the old bond. */
	    rc = ble_gap_conn_find(event->repeat_pairing.conn_handle, &desc);
	    assert(rc == 0);
	    ble_store_util_delete_peer(&desc.peer_id_addr);

	    /* Return BLE_GAP_REPEAT_PAIRING_RETRY to indicate that the host should
	     * continue with the pairing operation.
	     */
	    return BLE_GAP_REPEAT_PAIRING_RETRY;

	default:
	    return 0;
	}
}
 
 void ble_master_connect(char *name)
{
	int rc;
	memset(scan_ble_name,0,sizeof(scan_ble_name));
	memcpy(scan_ble_name,name,strlen(name));
	iot_printf("scan_ble_name=%s\n",scan_ble_name);
	uint8_t own_addr_type;
	struct ble_gap_disc_params disc_params;
	 /* Tell the controller to filter duplicates; we don't want to process
	 * repeated advertisements from the same device.
	 */
	disc_params.filter_duplicates = 0;

	/**
	 * Perform a passive scan.	I.e., don't send follow-up scan requests to
	 * each advertiser.
	 0x00 Passive Scanning. No scan request PDUs shall be sent.
	 0x01 Active Scanning. Scan request PDUs may be sent.
	 */
	disc_params.passive = 0;

	/* Use defaults for the rest of the parameters. */
	/*Time interval from when the Controller started its last scan until it 
	begins the subsequent scan on the primary advertising physical channel.
	Range: 0x0004 to 0xFFFF
	Time = N * 0.625 ms
	Time Range: 2.5 ms to 40.959375 s*/
	disc_params.itvl = 100;//æ‰«æé—´éš”ï¼Œè®¾ç½®å¤šä¹…æ‰«æä¸€æ¬¡  
	
	/* Duration of the scan on the primary advertising physical channel.
	Range: 0x0004 to 0xFFFF
	Time = N * 0.625 ms
	Time Range: 2.5 ms to 40.959375 s*/
	disc_params.window = 20;//扫描窗口时间,

	/*
	Scanning_Filter_Policy:
	Value Parameter Description
	0x00 Basic unfiltered scanning filter policy
	0x01 Basic filtered scanning filter policy
	0x02 Extended unfiltered scanning filter policy
	0x03 Extended filtered scanning filter policy
	All other values Reserved for future use
	*/
	disc_params.filter_policy = 0;
	disc_params.limited = 0;
	rc=ble_hs_scan_set_name_filt(strlen(scan_ble_name), scan_ble_name);
	if (rc != 0) {
		iot_printf("Error ble_hs_scan_set_name_filt rc=%d\n",rc);
	}
	rc = ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &disc_params,
					  ble_master_gap_event, NULL);
	if (rc != 0) {
		iot_printf("Error initiating GAP discovery procedure; rc=%d\n",rc);
	}


}
int ble_master_connect_addr(uint8_t *addr,uint8_t addr_type)
{
	int i,rc;
	int own_addr_type;
	ble_addr_t peer_addr;
	int32_t duration_ms;
	struct ble_gap_conn_params phy_1M_params = {0};
	memcpy(peer_addr.val,addr,6);
	printf("ble_master_connect_addr_type=%d    addr= ",addr_type);
	for (i = 0; i < 6; i ++) 
	{
	    printf("0x%x ", peer_addr.val[i]);
	}
	iot_printf("\n");
	own_addr_type=BLE_ADDR_PUBLIC;//ʹ�ù�����ַ����
	peer_addr.type=addr_type;//�ӻ���ַ����
	duration_ms=INT32_MAX;
	phy_1M_params.itvl_min=BLE_GAP_INITIAL_CONN_ITVL_MIN;
	 phy_1M_params.itvl_max=BLE_GAP_INITIAL_CONN_ITVL_MAX;
	 phy_1M_params.scan_window=0x0010;
	 phy_1M_params.scan_itvl=0x0010;
	 phy_1M_params.latency=0;
	 phy_1M_params.supervision_timeout=0x0100;
	 phy_1M_params.min_ce_len=0x0010;
	 phy_1M_params.max_ce_len=0x0300;

	rc=ble_gap_connect(own_addr_type, &peer_addr, duration_ms, &phy_1M_params,
	   		 ble_master_gap_event, NULL);
	if(rc)
		printf("ble_gap_connect error=%d  \n ",rc);
	return rc;
}					
