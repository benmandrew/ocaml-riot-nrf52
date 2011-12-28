#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <thread.h>
#include <msg.h>

#include <transceiver.h>
#include <radio/types.h>

#define PAYLOAD_SIZE  (0)

/* supported transceivers */
#ifdef MODULE_CC110X_NG
#include <cc110x_ng.h>
#if (CC1100_MAX_DATA_LENGTH > PAYLOAD_SIZE)
    #undef PAYLOAD_SIZE
    #define PAYLOAD_SIZE (CC1100_MAX_DATA_LENGTH)
#endif
#endif

//#define ENABLE_DEBUG (1)
#include <debug.h>

/*------------------------------------------------------------------------------------*/
/* used transceiver types */
transceiver_type_t transceivers = TRANSCEIVER_NONE;

/* registered upper layer threads */
registered_t reg[TRANSCEIVER_MAX_REGISTERED];

/* packet buffers */
radio_packet_t transceiver_buffer[TRANSCEIVER_BUFFER_SIZE];
uint8_t data_buffer[TRANSCEIVER_BUFFER_SIZE * PAYLOAD_SIZE];

/* message buffer */
msg_t msg_buffer[TRANSCEIVER_MSG_BUFFER_SIZE];

uint32_t response; ///< response bytes for messages to upper layer threads

int transceiver_pid; ///< the transceiver thread's pid

static volatile uint8_t rx_buffer_pos = 0;
static volatile uint8_t transceiver_buffer_pos = 0;

/* transceiver stack */
char transceiver_stack[TRANSCEIVER_STACK_SIZE];

/*------------------------------------------------------------------------------------*/
/* function prototypes */
static void run(void);
static void receive_packet(uint16_t type, uint8_t pos);
static void receive_cc110x_packet(radio_packet_t *trans_p);
static uint8_t send_packet(transceiver_type_t t, void *pkt);
static int16_t get_channel(transceiver_type_t t);
static int16_t set_channel(transceiver_type_t t, void *channel);
static int16_t get_address(transceiver_type_t t);
static int16_t set_address(transceiver_type_t t, void *address);
static void set_monitor(transceiver_type_t t, void *mode);
static void powerdown(transceiver_type_t t);
static void switch_to_rx(transceiver_type_t t);

/*------------------------------------------------------------------------------------*/
/* Transceiver init */
void transceiver_init(transceiver_type_t t) {
    uint8_t i;
    for (i = 0; i < TRANSCEIVER_MAX_REGISTERED; i++) {
        reg[i].transceivers = TRANSCEIVER_NONE;
        reg[i].pid          = 0;
    }
    if (t & TRANSCEIVER_CC1100) {
        transceivers |= t;
    }
    else {
        puts("Invalid transceiver type");
    }
}

/* Start the transceiver thread */
int transceiver_start(void) {
    transceiver_pid = thread_create(transceiver_stack, TRANSCEIVER_STACK_SIZE, PRIORITY_MAIN-3, CREATE_STACKTEST, run, "Transceiver");
    if (transceiver_pid < 0) {
        puts("Error creating transceiver thread");
    }
    else if (transceivers & TRANSCEIVER_CC1100) {
        DEBUG("Transceiver started for CC1100\n");
        cc110x_init(transceiver_pid);
    }
    return transceiver_pid;
}

/* Register an upper layer thread */
uint8_t transceiver_register(transceiver_type_t t, int pid) {
   uint8_t i;
   for (i = 0; ((reg[i].pid != pid) && 
               (i < TRANSCEIVER_MAX_REGISTERED) && 
               (reg[i].transceivers != TRANSCEIVER_NONE)); i++);

   if (i >= TRANSCEIVER_MAX_REGISTERED) {
       return ENOMEM;
   }
   else {
       reg[i].transceivers |= t;
       reg[i].pid         = pid;
       DEBUG("Thread %i registered for %i\n", reg[i].pid, reg[i].transceivers);
       return 1;
   }
}

/*------------------------------------------------------------------------------------*/
/*                                Internal functions                                  */
/*------------------------------------------------------------------------------------*/

/*
 * @brief The main thread run, receiving and processing messages in an infinite
 * loop
 */
void run(void) {
    msg_t m;
    transceiver_command_t *cmd;

    msg_init_queue(msg_buffer, TRANSCEIVER_MSG_BUFFER_SIZE);
    while (1) {
        msg_receive(&m);
        /* only makes sense for messages for upper layers */
        cmd = (transceiver_command_t*) m.content.ptr;

        DEBUG("Transceiver: Message received\n");
        switch (m.type) {
            case RCV_PKT_CC1020:
            case RCV_PKT_CC1100:
                receive_packet(m.type, m.content.value);
                break;
            case SND_PKT:
                response = send_packet(cmd->transceivers, cmd->data);
                m.content.value = response;
                msg_reply(&m, &m);
                break;
            case GET_CHANNEL:
                *((int16_t*) cmd->data) = get_channel(cmd->transceivers);
                msg_reply(&m, &m);
                break;
            case SET_CHANNEL:
                *((int16_t*) cmd->data) = set_channel(cmd->transceivers, cmd->data);
                msg_reply(&m, &m);
                break;
            case GET_ADDRESS:
                *((int16_t*) cmd->data) = get_address(cmd->transceivers);
                msg_reply(&m, &m);
                break;
            case SET_ADDRESS:
                *((int16_t*) cmd->data) = set_address(cmd->transceivers, cmd->data);
                msg_reply(&m, &m);
                break;
            case SET_MONITOR:
                set_monitor(cmd->transceivers, cmd->data);
                break;
            case POWERDOWN:
                powerdown(cmd->transceivers);
                break;
            case SWITCH_RX:
                switch_to_rx(cmd->transceivers);
                break;
            default:
                DEBUG("Unknown message received\n");
                break;
        }
    }
}

/*------------------------------------------------------------------------------------*/
/*
 * @brief Processes a packet received by any transceiver device
 *
 * @param type  The message type to determine which device has received the
 * packet
 * @param pos   The current device driver's buffer position
 */
static void receive_packet(uint16_t type, uint8_t pos) {
    uint8_t i = 0;
    transceiver_type_t t;
    rx_buffer_pos = pos;
    msg_t m;
   
    DEBUG("Packet received\n");
    switch (type) {
        case RCV_PKT_CC1020:
            t = TRANSCEIVER_CC1020;
            break;
        case RCV_PKT_CC1100:
            t = TRANSCEIVER_CC1100;
            break;
        default:
            t = TRANSCEIVER_NONE;
            break;
    }

    /* search first free position in transceiver buffer */
    for (i = 0; (i < TRANSCEIVER_BUFFER_SIZE) && (transceiver_buffer[transceiver_buffer_pos].processing); i++) {
        if (++transceiver_buffer_pos == TRANSCEIVER_BUFFER_SIZE) {
            transceiver_buffer_pos = 0;
        }
    }
    /* no buffer left */
    if (i >= TRANSCEIVER_BUFFER_SIZE) {
        /* inform upper layers of lost packet */
        m.type = ENOBUFFER;
        m.content.value = t;
    }
    /* copy packet and handle it */
    else {
        radio_packet_t *trans_p = &(transceiver_buffer[transceiver_buffer_pos]);
        m.type = PKT_PENDING;

        if (type == RCV_PKT_CC1100) {
            receive_cc110x_packet(trans_p); 
        }
        else {
            puts("Invalid transceiver type");
            return;
        }
    }

    /* finally notify waiting upper layers
     * this is done non-blocking, so packets can get lost */
    i = 0;
    while (reg[i].transceivers != TRANSCEIVER_NONE) {
        if (reg[i].transceivers & t) {
            m.content.ptr = (char*) &(transceiver_buffer[transceiver_buffer_pos]);
            DEBUG("Notify thread %i\n", reg[i].pid);
            if (msg_send(&m, reg[i].pid, false) && (m.type != ENOBUFFER)) {
                transceiver_buffer[transceiver_buffer_pos].processing++;
            }
        }
        i++;
    }
}

/*
 * @brief process packets from CC1100
 *
 * @param trans_p   The current entry in the transceiver buffer
 */
static void receive_cc110x_packet(radio_packet_t *trans_p) {
    DEBUG("Handling CC1100 packet\n");
    /* disable interrupts while copying packet */
    dINT();
    cc110x_packet_t p = cc110x_rx_buffer[rx_buffer_pos].packet;
    
    trans_p->src = p.phy_src;
    trans_p->dst = p.address;
    trans_p->rssi = cc110x_rx_buffer[rx_buffer_pos].rssi;
    trans_p->lqi = cc110x_rx_buffer[rx_buffer_pos].lqi;
    trans_p->length = p.length - CC1100_HEADER_LENGTH;
    memcpy((void*) &(data_buffer[transceiver_buffer_pos * PAYLOAD_SIZE]), p.data, CC1100_MAX_DATA_LENGTH);
    eINT();

    DEBUG("Packet %p was from %hu to %hu, size: %u\n", trans_p, trans_p->src, trans_p->dst, trans_p->length);
    trans_p->data = (uint8_t*) &(data_buffer[transceiver_buffer_pos * CC1100_MAX_DATA_LENGTH]);
}
 
/*------------------------------------------------------------------------------------*/
/*
 * @brief Sends a radio packet to the receiver
 *
 * @param t     The transceiver device
 * @param pkt   Generic pointer to the packet
 *
 * @return 1 on success, 0 otherwise
 */
static uint8_t send_packet(transceiver_type_t t, void *pkt) {
    uint8_t res = 0;
    radio_packet_t p = *((radio_packet_t*) pkt);
    cc110x_packet_t cc110x_pkt;

    switch (t) {
        case TRANSCEIVER_CC1100:
            cc110x_pkt.length = p.length + CC1100_HEADER_LENGTH;
            cc110x_pkt.address = p.dst;
            cc110x_pkt.flags = 0;
            memcpy(cc110x_pkt.data, p.data, p.length);

            res = cc110x_send(&cc110x_pkt);
            break;
        default:
            puts("Unknown transceiver");
            break;
    }
    return res;
}

/*------------------------------------------------------------------------------------*/
/*
 * @brief Sets the radio channel for any transceiver device
 *
 * @param t         The transceiver device
 * @param channel   The channel to be set
 *
 * @return The radio channel AFTER calling the set command, -1 on error
 */
static int16_t set_channel(transceiver_type_t t, void *channel) {
    uint8_t c = *((uint8_t*) channel);
    switch (t) {
        case TRANSCEIVER_CC1100:
            return cc110x_set_channel(c);
        default:
            return -1;
    }
}

/*
 * @brief Get the radio channel of any transceiver device
 *
 * @param t     The transceiver device
 *
 * @return The current radio channel of the transceiver, -1 on error
 */
static int16_t get_channel(transceiver_type_t t) {
    switch (t) {
        case TRANSCEIVER_CC1100:
            return cc110x_get_channel();
        default:
            return -1;
    }
}
/*------------------------------------------------------------------------------------*/
/*
 * @brief Get the current address of transceiver device
 *
 * @param t     The transciever device
 *
 * @return  The configured address of the device, -1 on error
 */
static int16_t get_address(transceiver_type_t t) {
    switch (t) {
        case TRANSCEIVER_CC1100:
            return cc110x_get_address();
        default:
            return -1;
    }
}

/*
 * @brief Set the address of the transceiver device
 *
 * @param t         The transceiver device
 * @param address   Generic pointer to the address to set
 *
 * @return  The new radio address of the device
 */
static int16_t set_address(transceiver_type_t t, void *address) {
    radio_address_t addr = *((radio_address_t*) address);
    switch (t) {
        case TRANSCEIVER_CC1100:
            return cc110x_set_address(addr);
        default:
            return -1;
    }
}

/*
 * @brief Set the transceiver device into monitor mode (disabling address check)
 *
 * @param t         The transceiver device
 * @param mode      1 for enabling monitor mode, 0 for enabling address check
 */
static void set_monitor(transceiver_type_t t, void *mode) {
    switch (t) {
        case TRANSCEIVER_CC1100:
            cc110x_set_monitor(*((uint8_t*) mode));
            break;
        default:
            break;
    }
}
/*------------------------------------------------------------------------------------*/
static void powerdown(transceiver_type_t t) {
    switch (t) {
        case TRANSCEIVER_CC1100:
            cc110x_switch_to_pwd();
            break;
        default:
            break;
    }
}

/*------------------------------------------------------------------------------------*/
static void switch_to_rx(transceiver_type_t t) {
    switch (t) {
        case TRANSCEIVER_CC1100:
            cc110x_switch_to_rx();
            break;
        default:
            break;
    }
}
