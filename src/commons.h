#ifndef XKAAPI_SIMU_COMMONS_H__
#define XKAAPI_SIMU_COMMONS_H__

#include <msg/msg.h>
#include "list.h"

#define bool int
#define true 1
#define false 0

struct xksimu_data_t;
struct xksimu_blocks_t;
struct xksimu_task_t;
struct xksimu_proc_t;
struct xksimu_host_t;
struct xksimu_com_t;

// ----- COM -----

struct xksimu_com_t {
        int pid;
        struct xksimu_host_t * host;    // Corresponding host
        msg_process_t process;     // Simgrid (MSG api) process
        struct xksimu_list_t * requests;         // Data => Process && Blocks
        struct xksimu_list_t * applicants;      // Process => Data
        // TODO : Sauvegarder messages, actions ...?...
};

struct xksimu_com_applicant_t {
        struct xksimu_proc_t * proc;
        struct xksimu_list_t * datas;
};

struct xksimu_com_data_t {
        bool requested;                  // 1 if requested t an other host, 0 if not
        struct xksimu_data_t * data;    // Data requested
        struct xksimu_list_t * blocks;  // Non available blocks
        struct xksimu_list_t * applicants;       // Applicant's process
        struct xksimu_list_t * awaiting_blocks;
};

enum xksimu_com_request_type_e {
        XKS_COM_REQ_STEAL_LOCAL,
        XKS_COM_REQ_STEAL_DISTANT,
        XKS_COM_REQ_STEAL_ANSWER,
        XKS_COM_REQ_DATA_LOCAL,
        XKS_COM_REQ_DATA_DISTANT,
        XKS_COM_REQ_DATA_ANSWER, 
        XKS_COM_REQ_BLOCK_DISTANT, 
        XKS_COM_REQ_BLOCK_ANSWER
};

struct xksimu_com_request_t {
        enum xksimu_com_request_type_e type;
        void * data;
};

struct xksimu_com_request_data_local_t {
        struct xksimu_proc_t * source;
        struct xksimu_list_t * data;
};

struct xksimu_com_request_steal_t {
        struct xksimu_proc_t * dest;
        struct xksimu_proc_t * source;
};

struct xksimu_com_request_steal_answer_t {
        struct xksimu_proc_t * dest;
        struct xksimu_proc_t * source;
        struct xksimu_task_t * data;
};

struct xksimu_com_request_data_t {
       struct xksimu_host_t * source;
       struct xksimu_data_t * data;
       struct xksimu_list_t * blocks;
};

struct xksimu_com_request_data_answer_t {
       struct xksimu_host_t * source;
       struct xksimu_data_t * data;
       struct xksimu_list_t * blocks;
       // struct xksimu_block_t * block;
};

struct xksimu_com_request_block_t {
        struct xksimu_host_t * source;
        struct xksimu_block_t * block;
};

// ----- HOST -----

struct xksimu_host_t {
        msg_host_t host;                        // Simgrig (MSG api) Host
        int id;                                 // Host ID
        const char * name;                      // Host name
        struct xksimu_com_t * com;              // Comm process
        struct xksimu_list_t data_blocks;       // Owned data blocks
        struct xksimu_list_t proc;              // Owned working process
};

// ----- PROC -----

struct xksimu_proc_t {
        msg_process_t process;          // Simgrid (MSG api) process
        int pid;                        // Process ID
        struct xksimu_list_t tasks;     // Owned tasks
        struct xksimu_host_t * host;    // Corresponding host
        int steal_request;              // Is the host waiting for the result of a steal request ?
};

// ----- DATA -----

struct xksimu_data_t {
        char id[20];                              // Data ID
        int version;                            // Data version
        int size;                               // Data size
        struct xksimu_list_t next_tasks;        // Tasks which required the data
        struct xksimu_list_t blocks;            // List of data blocks
};

// -- DATA.BLOCKS --

struct xksimu_block_t {
        int id;                         // Id of the block
        int size;                       // Size of the block
        struct xksimu_data_t * data;      // Corespondig data
        struct xksimu_list_t owners;    // Hosts which have the block
};

// ----- TASK -----

enum xksimu_task_state_e {
        non_available,          // Non generated
        available,              // Generated, data non available
        fully_available,        // Generated, data available
        done,                   // Done
        started                 // Started but not done (waiting data per example)
};

struct xksimu_task_t {
        char id[15];                    // Task's ID
        int time;                       // Computation time for the task
        msg_task_t task;                // Simgrid (MSG api) task
        struct xksimu_list_t require;   // Data required to compute
        struct xksimu_list_t provide;   // Data provided after computation
        enum xksimu_task_state_e state; // Task state
};

// ----- Divers -----

int rand_n(int n);

#endif
