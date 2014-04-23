#include <msg/msg.h>
#include <xbt/log.h>
#include <xbt/asserts.h>

#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include "commons.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(XKSIMU, "Messages speficic to the xKaapi simulation.");
XBT_LOG_NEW_SUBCATEGORY(XKSIMU_TASKS, XKSIMU, "Tasks' relatives messages.");
XBT_LOG_NEW_SUBCATEGORY(XKSIMU_DATAS, XKSIMU, "Datas' relatives messages.");
XBT_LOG_NEW_SUBCATEGORY(XKSIMU_INIT, XKSIMU, "Init relatives messages.");

// ----- PROTOTYPES -----

int init_simulation (int argc, char *argv[]);
int worker (int argc, char *argv[]);
int comm (int argc, char *argv[]);

void create_tasks(const char * filename);
void create_datas(const char * filename);
void init_tasks(const char * filename);
void init_datas(const char * filename);
void create_host(void);

struct xksimu_data_t * get_data(char * id);
struct xksimu_task_t * get_task(char * id);

struct xksimu_host_t * host_get(msg_host_t host);
struct xksimu_proc_t * proc_get(struct xksimu_host_t * host, msg_process_t proc);

struct xksimu_task_t * get_fullyavailable_task(struct xksimu_proc_t * proc);
struct xksimu_task_t * get_available_task(struct xksimu_proc_t * proc);
struct xksimu_list_t * get_missingdata_list(struct xksimu_proc_t * proc, struct xksimu_task_t * task);

struct xksimu_com_data_t * get_com_data(struct xksimu_host_t * host, struct xksimu_data_t * data);
void * get_first_intersection(struct xksimu_list_t * list1, struct xksimu_list_t * list2);

// ----- GLOBAL VARIABLES -----

struct xksimu_host_t * hosts_table;
int nb_hosts;
struct xksimu_data_t * datas_table;
int nb_datas;
struct xksimu_task_t * tasks_table;
int nb_tasks;
struct xksimu_list_t remaining_tasks;
struct xksimu_list_t running_workers;
int block_size;

// ----- FUNCTIONS -----

/* Retrieve the host corresponding to the Simgrid host host. */
struct xksimu_host_t * host_get(msg_host_t host) {
        const char * name = MSG_host_get_name(host);
        for (int i = 0; i < nb_hosts; i++) {
                if (strcmp(name, hosts_table[i].name) == 0) {
                        return &(hosts_table[i]);
                }
        }

        return NULL;
}

/* Retrieve the process corresponding to the Simgrid process proc on host host. */
struct xksimu_proc_t * proc_get(struct xksimu_host_t * host, msg_process_t proc) {
        int pid = MSG_process_get_PID(proc);

        struct xksimu_proc_t * p;
        struct xksimu_list_el_t * el;
        xksimu_list_foreach (host->proc, el, p) {
                if (p->pid == pid) {
                        return p;
                }
        }

        return NULL;
}

/* Retrieve tje com_data correspondind to a given data and a given host. */
struct xksimu_com_data_t * get_com_data(struct xksimu_host_t * host, struct xksimu_data_t * data) {
        struct xksimu_com_data_t * d;
        struct xksimu_list_el_t * el;
        xksimu_list_foreach(*(host->com->requests), el, d) {
                if (d->data == data) {
                        return d;
                }
        }

        return NULL;
}

/* Return a task available without its data for the process proc. */
struct xksimu_task_t * get_available_task(struct xksimu_proc_t * proc) {
        struct xksimu_task_t * t;
        struct xksimu_list_el_t * el;

        xksimu_list_foreach (proc->tasks, el, t) {
                if (t->state == available) {
                        return t;
                }
        }

        return NULL;
}

/* Return a task available with its data for the process proc. */
struct xksimu_task_t * get_fullyavailable_task(struct xksimu_proc_t * proc) {
        struct xksimu_task_t * t;
        struct xksimu_list_el_t * el;

        xksimu_list_foreach (proc->tasks, el, t) {
                if (t->state == fully_available) {
                        return t;
                }
        }

        return NULL;
}

struct xksimu_data_t * get_data(char * id) {
        for (int i = 0 ; i < nb_datas ; i++) {
                if (strcmp(datas_table[i].id, id) == 0) {
                        return &(datas_table[i]);
                }
        }
        
        XBT_ERROR("Data %s not found.", id);
        return NULL;
}

struct xksimu_task_t * get_task(char * id) {
        for (int i = 0 ; i < nb_tasks ; i++) {
                if (strcmp(tasks_table[i].id, id) == 0) {
                        return &(tasks_table[i]);
                }
        }

        XBT_ERROR("Task %s not found.", id);
        return NULL;
}

void * get_first_intersection(struct xksimu_list_t * list1, struct xksimu_list_t * list2) {
        void * d;
        struct xksimu_list_el_t * el;

        xksimu_list_foreach (*list1, el, d) {
                if (xksimu_list_contains(*list2, d)) {
                        return d;
                }
        }

        return NULL;
}

/* Return the list of the missig (or partial) data of task on process proc. */
struct xksimu_list_t * get_missingdata_list(struct xksimu_proc_t * proc, struct xksimu_task_t * task) {
        struct xksimu_list_t * res = calloc(1, sizeof(struct xksimu_list_t));

        res->head = NULL;
        res->queue = NULL;
        res->size = 0;

        struct xksimu_list_el_t * el1;
        struct xksimu_data_t * d;
        xksimu_list_foreach (task->require, el1, d) {
                struct xksimu_list_el_t * el2;
                struct xksimu_block_t * b;
                xksimu_list_foreach(d->blocks, el2, b) {
                        if(! xksimu_list_contains(b->owners, proc->host)) {
                                xksimu_list_push_front(res, d);
                                break;
                        }
                }
        }

        return res;
}

// ----- INIT FUNCTIONS -----

void create_tasks(const char * filename) {
        FILE * file = fopen(filename, "r");
        char buf[256];
        char * str;
        int n_di;
        int n_do;
        int n_ct;

        str = fgets(buf, 256, file);
        buf[strlen(buf) - 1] = '\0';
        if (str != NULL) {
                nb_tasks = atoi(buf);
        }
        tasks_table = calloc(nb_tasks, sizeof(struct xksimu_task_t));

        for (int i = 0 ; i < nb_tasks ; i++) {
                str = fgets(buf, 256, file);
                buf[strlen(buf) - 1] = '\0';
                sprintf(tasks_table[i].id, "%s", buf);

                if (strcmp("init", tasks_table[i].id) == 0) {
                        tasks_table[i].state = fully_available;
                        struct xksimu_proc_t * proc = hosts_table[0].proc.head->data;
                        xksimu_list_push_front(&(proc->tasks), &(tasks_table[i]));
                }

                str = fgets(buf, 256, file);
                buf[strlen(buf) - 1] = '\0';
                tasks_table[i].time = atoi(buf);
                str = fgets(buf, 256, file); // creator
                buf[strlen(buf) - 1] = '\0';
                
                str = fgets(buf, 256, file); // nb data input
                buf[strlen(buf) - 1] = '\0';
                n_di = atoi(buf);

                for (int j = 0 ; j < n_di ; j++) {
                        str = fgets(buf, 256, file);
                }
               
                if (n_di == 0) {
                        str = fgets(buf, 256, file);
                }
                
                str = fgets(buf, 256, file); // nb data out
                buf[strlen(buf) - 1] = '\0';
                n_do = atoi(buf);

                for (int j = 0 ; j < n_do ; j++) {
                        str = fgets(buf, 256, file);
                }
               
                if (n_do == 0) {
                        str = fgets(buf, 256, file);
                }
                
                str = fgets(buf, 256, file); // nb created tasks
                buf[strlen(buf) - 1] = '\0';
                n_ct = atoi(buf);

                for (int j = 0 ; j < n_ct ; j++) {
                        str = fgets(buf, 256, file);
                }
               
                if (n_ct == 0) {
                        str = fgets(buf, 256, file);
                }
                
                // TODO : Computation time in flop instead of 50000000
                // TODO : Communication size in byte instead of 100 => Concrètement, lors du vol il y a quoi qui circule ?
                tasks_table[i].task = MSG_task_create(tasks_table[i].id, 900000000000, 100, &(tasks_table[i]));
                xksimu_list_push_front(&remaining_tasks, &tasks_table[i]);
                
                XBT_CINFO(XKSIMU_INIT, "Task %s created (di : %d - do : %d - ct: %d)", tasks_table[i].id, n_di, n_do, n_ct);
        }
        fclose(file);
}

void init_tasks(const char * filename) {
        FILE * file = fopen(filename, "r");
        char buf[256];
        char * str;
        int n_di;
        int n_do;
        int n_ct;

        str = fgets(buf, 256, file);

        for (int i = 0 ; i < nb_tasks ; i++) {
                XBT_CINFO(XKSIMU_INIT, "Links added for task %s.", tasks_table[i].id);
                
                str = fgets(buf, 256, file); // id
                str = fgets(buf, 256, file); // time
                str = fgets(buf, 256, file); // creator
                
                str = fgets(buf, 256, file); // nb data input
                buf[strlen(buf) - 1] = '\0';
                n_di = atoi(buf);

                for (int j = 0 ; j < n_di ; j++) {
                        str = fgets(buf, 256, file);
                        buf[strlen(buf) - 1] = '\0';
                        struct xksimu_data_t * data = get_data(buf);
                        xksimu_list_push_front(&(tasks_table[i].require), data);
                        XBT_CVERB(XKSIMU_INIT, "      > Required data added : %s -> %s.", buf, data->id);
                }
               
                if (n_di == 0) {
                        str = fgets(buf, 256, file);
                }
                
                str = fgets(buf, 256, file); // nb data out
                buf[strlen(buf) - 1] = '\0';
                n_do = atoi(buf);

                for (int j = 0 ; j < n_do ; j++) {
                        str = fgets(buf, 256, file);
                        buf[strlen(buf) - 1] = '\0';
                        struct xksimu_data_t * data = get_data(buf);
                        xksimu_list_push_front(&(tasks_table[i].provide), data);
                        XBT_CVERB(XKSIMU_INIT, "      > Provided data added : %s -> %s.", buf, data->id);
                }
               
                if (n_do == 0) {
                        str = fgets(buf, 256, file);
                }
                
                str = fgets(buf, 256, file); // nb created tasks
                buf[strlen(buf) - 1] = '\0';
                n_ct = atoi(buf);

                for (int j = 0 ; j < n_ct ; j++) {
                        str = fgets(buf, 256, file);
                }
               
                if (n_ct == 0) {
                        str = fgets(buf, 256, file);
                }
        }
        fclose(file);
}

void create_datas(const char * filename) {
        FILE * file = fopen(filename, "r");
        char buf[256];
        char * str;
        int n_dt;

        str = fgets(buf, 256, file);
        buf[strlen(buf) - 1] = '\0';
        if (str != NULL) {
                nb_datas = atoi(buf);
        }

        datas_table = calloc(nb_datas, sizeof(struct xksimu_data_t));

        for (int i = 0 ; i < nb_datas ; i++) {
                datas_table[i].blocks.head = NULL;
                datas_table[i].blocks.queue = NULL;
                datas_table[i].blocks.size = 0;
                
                str = fgets(buf, 256, file); // id
                buf[strlen(buf) - 1] = '\0';
                sprintf(datas_table[i].id, "%s", buf);

                str = fgets(buf, 256, file); // size
                buf[strlen(buf) - 1] = '\0';
                datas_table[i].size = atoi(buf);

                int size = datas_table[i].size;
                if (size <= 0) {
                        struct xksimu_block_t * block = calloc(1, sizeof(struct xksimu_block_t));
                        block->id = 0;
                        block->size = 1;
                        block->data = &(datas_table[i]);
                        XBT_CDEBUG(XKSIMU_INIT, "Adding block %d (%do).", 0, block->size);
                        xksimu_list_push_front(&(datas_table[i].blocks), block);
                } else {
                        for (int j = 0 ; size > 0 ; j++, size -= block_size) {
                                struct xksimu_block_t * block = calloc(1, sizeof(struct xksimu_block_t));
                                block->id = j;
                                block->size = size - block_size > 0 ? block_size : size;
                                block->data = &(datas_table[i]);
                                XBT_CDEBUG(XKSIMU_INIT, "Adding block %d (%do).", j, block->size);
                                xksimu_list_push_front(&(datas_table[i].blocks), block);
                        }
                }

                str = fgets(buf, 256, file); // version
                buf[strlen(buf) - 1] = '\0';
                datas_table[i].version = atoi(buf + 1);

                str = fgets(buf, 256, file); // producer

                str = fgets(buf, 256, file); // depending tasks
                buf[strlen(buf) - 1] = '\0';
                n_dt = atoi(buf);

                for (int j = 0 ; j < n_dt ; j++) {
                        str = fgets(buf, 256, file);
                }
               
                if (n_dt == 0) {
                        str = fgets(buf, 256, file);
                }
                
                XBT_CINFO(XKSIMU_INIT, "Data %s created (size : %d - version : %d)", datas_table[i].id, datas_table[i].size, datas_table[i].version);
        }

        fclose(file);
}

void init_datas(const char * filename) {
        FILE * file = fopen(filename, "r");
        char buf[256];
        char * str;
        int n_dt;

        str = fgets(buf, 256, file);

        for (int i = 0 ; i < nb_datas ; i++) {
                XBT_CINFO(XKSIMU_INIT, "Links added for data %s.", datas_table[i].id);
                
                str = fgets(buf, 256, file); // id
                str = fgets(buf, 256, file); // size
                str = fgets(buf, 256, file); // version
                str = fgets(buf, 256, file); // producer
                
                str = fgets(buf, 256, file); // depending tasks
                buf[strlen(buf) - 1] = '\0';
                n_dt = atoi(buf);

                for (int j = 0 ; j < n_dt ; j++) {
                        str = fgets(buf, 256, file);
                        buf[strlen(buf) - 1] = '\0';
                        struct xksimu_task_t * task = get_task(buf);
                        xksimu_list_push_front(&(datas_table[i].next_tasks), task);
                        XBT_CVERB(XKSIMU_INIT, "      > Next Task added : %s -> %s.", buf, task->id);
                }
               
                if (n_dt == 0) {
                        str = fgets(buf, 256, file);
                }
        }
        fclose(file);
}

void create_host(void) {
        unsigned int i;
        xbt_dynar_t hosts = MSG_hosts_as_dynar();
        nb_hosts = xbt_dynar_length(hosts);
        msg_host_t host;

        XBT_CINFO(XKSIMU_INIT, "%d hosts detected.", nb_hosts);

        hosts_table = calloc(nb_hosts, sizeof(struct xksimu_host_t));

        xbt_dynar_foreach (hosts,i,host) {
                hosts_table[i].id = i;
                hosts_table[i].host = host;
                hosts_table[i].name = MSG_host_get_name(host); 
                XBT_CVERB(XKSIMU_INIT, "Host number %d : %s added.", i, hosts_table[i].name);

                xbt_swag_t process = MSG_host_get_process_list(host);
                XBT_CDEBUG(XKSIMU_INIT, "%d process attached to it.", xbt_swag_size(process));
                msg_process_t proc;
                xbt_swag_foreach(proc, process) {
                        if (strcmp(MSG_process_get_name(proc), "worker") == 0) {
                                struct xksimu_proc_t * curr_proc = calloc(1, sizeof(struct xksimu_proc_t));
                                curr_proc->host = &(hosts_table[i]);
                                curr_proc->process = proc;
                                curr_proc->pid = MSG_process_get_PID(proc);
                                curr_proc->steal_request = false;
                                xksimu_list_push_front(&(hosts_table[i].proc), curr_proc);
                                xksimu_list_push_front(&running_workers, curr_proc);
                                XBT_CDEBUG(XKSIMU_INIT, "Adding a worker process, pid : %d.", curr_proc->pid);
                        } else if (strcmp(MSG_process_get_name(proc), "comm") == 0) {
                                struct xksimu_com_t * curr_proc = calloc(1, sizeof(struct xksimu_com_t));
                                curr_proc->host = &(hosts_table[i]);
                                curr_proc->process = proc;
                                curr_proc->pid = MSG_process_get_PID(proc);
                                curr_proc->requests = calloc(1, sizeof(struct xksimu_list_t));
                                curr_proc->applicants = calloc(1, sizeof(struct xksimu_list_t));

                                hosts_table[i].com = curr_proc;
                                XBT_CDEBUG(XKSIMU_INIT, "Adding a comm process, pid : %d.", curr_proc->pid);
                        }
                }
        } 
}

int init_simulation (int argc, char *argv[]) {
        const char * tasks = "graph.tasks";
        const char * datas = "graph.data";

        if (argc > 1) {
                tasks = argv[0];
                datas = argv[1];
        } else {
                XBT_INFO("No graph provided, using graph.tasks and graph.data");
        }

        if (argc > 2) {
                block_size = atoi(argv[2]);
        } else {
                block_size = 1000;
        }

        create_host();
        create_tasks(tasks); // Just create the tasks
        create_datas(datas); // Just create the data
        init_tasks(tasks);   // Add the data links to the tasks
        init_datas(datas);   // Add the task links to the datas
        
        return 0;
}

// ----- SIMULATION PROCESS -----

int worker (int argc, char *argv[]) {
        struct xksimu_host_t * host = host_get(MSG_host_self());
        struct xksimu_proc_t * proc = proc_get(host, MSG_process_self());
        _XBT_GNUC_UNUSED void * res;
        
        while (remaining_tasks.head != NULL) {
                struct xksimu_task_t * task = get_fullyavailable_task(proc);
                if (task != NULL) {
                        XBT_CINFO(XKSIMU_TASKS, "[Execute] Task : num %s.", task->id);

                        res = xksimu_list_remove(&(proc->tasks), task);
                        res = xksimu_list_remove(&remaining_tasks, task);

                        task->state = started;
                        MSG_task_execute(task->task);
                        MSG_task_destroy(task->task);
                        
                        // On met toutes les données générées dans l'état dispo
                        struct xksimu_data_t * d;
                        struct xksimu_list_el_t * el0;
                        xksimu_list_foreach(task->provide, el0, d) {
                                XBT_CVERB(XKSIMU_DATAS, "Generated data : %s, v%d", d->id, d->version);
                                // L'host courant possède tous les blocs de la data
                                struct xksimu_block_t * b;
                                struct xksimu_list_el_t * el1;

                                xksimu_list_foreach(d->blocks, el1, b) {
                                        xksimu_list_push_front(&(b->owners), host);
                                        xksimu_list_push_front(&(host->data_blocks), b);
                                }

                                XBT_CDEBUG(XKSIMU_DATAS, "All blocks added for data %s and host %p.", d->id, host);

                                // On regarde si la génération de cette donnée active une tache.
                                struct xksimu_task_t * t;
                                xksimu_list_foreach(d->next_tasks, el1, t) {
                                        bool ok = true;
                                        struct xksimu_data_t * d_req;
                                        struct xksimu_list_el_t * el2;
                                        xksimu_list_foreach (t->require, el2, d_req) {
                                                struct xksimu_block_t * first_block = d_req->blocks.head->data;
                                                struct xksimu_list_el_t * first_owner = first_block->owners.head;
                                                if (first_owner == NULL) {
                                                        XBT_CDEBUG(XKSIMU_DATAS, "Data %s still missing for task %s.", d_req->id, t->id);
                                                        ok = false;
                                                        break;
                                                }
                                        }

                                        if (ok) {
                                                XBT_CVERB(XKSIMU_TASKS, "Task %s activated and added to %s/%d.", t->id, host->name, proc->pid);
                                                t->state = available;
                                                xksimu_list_push_front(&(proc->tasks), t);
                                        }
                                }
                        }
                        task->state = done;
                } else {
                        struct xksimu_task_t * task_av = get_available_task(proc);
                        if (task_av != NULL) {
                                struct xksimu_list_t * missing_data = get_missingdata_list(proc, task_av);
                                if (xksimu_list_empty(missing_data)) {
                                        task_av->state = fully_available;
                                } else {
                                        // No one should steal the task, we are waiting the data to start it.
                                        task_av->state = started;

                                        struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                        struct xksimu_com_request_data_local_t * data_request = calloc(1, sizeof(struct xksimu_com_request_data_local_t));
                                        com_request->type = XKS_COM_REQ_DATA_LOCAL;
                                        com_request->data = data_request;
                                        data_request->source = proc;
                                        data_request->data = missing_data;

                                        msg_task_t message = MSG_task_create(NULL, 100, 100, com_request);
                                        MSG_task_set_category(message, "LOCAL");
                                        MSG_task_send(message, host->name);

                                        XBT_CVERB(XKSIMU_DATAS, "[Request][Local] Data required for task %s.", task_av->id);
                                        // Waiting for the datas
                                        MSG_process_suspend(proc->process);
                                        // Datas are now available
                                        task_av->state = fully_available;
                                }
                        } else {
                                struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                struct xksimu_com_request_steal_t * steal_request = calloc(1, sizeof(struct xksimu_com_request_steal_t));
                                com_request->type = XKS_COM_REQ_STEAL_LOCAL;
                                com_request->data = steal_request;
                                steal_request->dest = NULL;
                                steal_request->source = proc;

                                XBT_CVERB(XKSIMU_TASKS, "Local steal request sent.");
                                msg_task_t message = MSG_task_create(NULL, 100, 100, com_request);
                                MSG_task_set_category(message, "LOCAL");
                                MSG_task_send(message, host->name);
                                MSG_process_suspend(proc->process);
                        }
                }
        }

        xksimu_list_remove(&running_workers, proc);
        
        return 0;
}

int comm (int argc, char *argv[]) {
        struct xksimu_host_t * host = host_get(MSG_host_self());
        _XBT_GNUC_UNUSED struct xksimu_com_t * com = host->com;
        _XBT_GNUC_UNUSED void * res;
        msg_error_t error;
        int n = 1;

        while (running_workers.head != NULL) {
                if (! MSG_task_listen(host->name)) {
                        n *= 2;
                        MSG_process_sleep(n);
                        continue;
                }
                n = n/2 > 0 ? n/2 : 1;

                msg_task_t message_r = NULL;
                error = MSG_task_receive(&message_r, host->name);

                if (error != MSG_OK) {
                        XBT_WARN("Receive failed : %d", error);
                        MSG_process_sleep(n);
                        continue;
                }

                struct xksimu_com_request_t * com_receive = MSG_task_get_data(message_r);
                MSG_task_destroy(message_r);

                switch(com_receive->type) {
                        case XKS_COM_REQ_STEAL_LOCAL: {
                                struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                struct xksimu_com_request_steal_t * steal_request_send = com_receive->data;

                                // Choosing a random host
                                struct xksimu_host_t * host_dest = NULL;
                                do {
                                        host_dest = &(hosts_table[rand_n(nb_hosts - 1)]);
                                } while (host_dest->id == host->id);
                               
                                // Chossing a random process
                                int _n = rand_n(host->proc.size-1) +1;
                                struct xksimu_proc_t * proc_dest = xksimu_list_get(host_dest->proc, _n);

                                XBT_CINFO(XKSIMU_TASKS, "[Request] Sending steal request from %d to %s/%d.", steal_request_send->source->pid, host_dest->name, proc_dest->pid);
                                
                                // On a reçut une demande locale, on la transmet à un hote choisit au hasard.
                                com_request->type = XKS_COM_REQ_STEAL_DISTANT;
                                com_request->data = steal_request_send;
                                steal_request_send->dest = proc_dest;

                                msg_task_t message_s = MSG_task_create(NULL, 100, 100, com_request);
                                MSG_task_set_category(message_s, "TASK");
                                MSG_task_isend(message_s, host_dest->name);
                                
                                break;
                        } case XKS_COM_REQ_STEAL_DISTANT: {
                                struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                struct xksimu_com_request_steal_answer_t * answer = calloc(1, sizeof(struct xksimu_com_request_steal_answer_t));
                                
                                struct xksimu_com_request_steal_t * steal_request = com_receive->data;

                                XBT_CVERB(XKSIMU_TASKS, "[Answer] Receive Steal Request from %s/%d for thread %d, first task : %p.", steal_request->source->host->name, steal_request->source->pid, steal_request->dest->pid, steal_request->dest->tasks.head);
                                
                                if (steal_request->dest->tasks.head != NULL) {
                                        XBT_CDEBUG(XKSIMU_TASKS, "%d remaining tasks, %d available tasks", remaining_tasks.size, steal_request->dest->tasks.size);
                                        XBT_CDEBUG(XKSIMU_TASKS, "      > head status : %d", ((struct xksimu_task_t *)steal_request->dest->tasks.head->data)->state);
                                        struct xksimu_task_t * _t = steal_request->dest->tasks.head->data;
                                        XBT_CDEBUG(XKSIMU_TASKS, "      > head task : %s", _t->id);
                                        XBT_CDEBUG(XKSIMU_TASKS, "      > head missing datas : %d", get_missingdata_list(steal_request->dest, (struct xksimu_task_t *)steal_request->dest->tasks.head->data)->size);
                                        struct xksimu_list_t * d = get_missingdata_list(steal_request->dest, (struct xksimu_task_t *)steal_request->dest->tasks.head->data);
                                        if (d->head != NULL) {
                                                struct xksimu_data_t * _d = d->head->data;
                                                XBT_CDEBUG(XKSIMU_TASKS, "      > missing data : %s", _d->id);
                                                struct xksimu_block_t * _b = _d->blocks.head->data;
                                                struct xksimu_list_el_t * _el = _b->owners.head;
                                                XBT_CDEBUG(XKSIMU_TASKS, "      > missing data owners");
                                                while (_el != NULL) {
                                                        struct xksimu_host_t * _h = _el->data;
                                                        XBT_CDEBUG(XKSIMU_TASKS, "              > %s", _h->name);
                                                        _el = _el->next;
                                                }
                                        }
                                }

                                com_request->type = XKS_COM_REQ_STEAL_ANSWER;
                                com_request->data = answer;
                                answer->dest = steal_request->source;
                                answer->source = steal_request->dest;
                                answer->data = get_available_task(steal_request->dest);
                                res = xksimu_list_remove(&(steal_request->dest->tasks), answer->data);

                                msg_task_t message_s = MSG_task_create(NULL, 100, 100, com_request);
                                MSG_task_set_category(message_s, "TASK");
                                MSG_task_isend(message_s, steal_request->source->host->name);

                                free(steal_request);
                                break;
                        } case XKS_COM_REQ_STEAL_ANSWER: {
                                struct xksimu_com_request_steal_answer_t * answer = com_receive->data;
                                struct xksimu_task_t * task = answer->data;

                                if (answer->data == NULL) {
                                        XBT_CINFO(XKSIMU_TASKS, "[Receive] No task available in process %s/%d.", answer->source->host->name, answer->source->pid);
                                } else {
                                        XBT_CINFO(XKSIMU_TASKS, "[Receive] Task %s receive from %s/%d for %d.", task->id, answer->source->host->name, answer->source->pid, answer->dest->pid);
                                        xksimu_list_push_back(&(answer->dest->tasks), task);
                                }

                                MSG_process_resume(answer->dest->process);

                                free(answer);
                                break;
                        } case XKS_COM_REQ_DATA_LOCAL: {
                                struct xksimu_com_request_data_local_t * data_request = com_receive->data;
                                struct xksimu_com_applicant_t * applicant = calloc(1, sizeof(struct xksimu_com_applicant_t));
                                struct xksimu_com_data_t * data = NULL;

                                XBT_CVERB(XKSIMU_DATAS, "[Receive] Local Data Request");

                                applicant->datas = calloc(1, sizeof(struct xksimu_list_t));
                                // Pour chaque donné, Chercher si la donnée a déjà été demandée.
                                struct xksimu_data_t * d;
                                struct xksimu_list_el_t * el;
                                xksimu_list_foreach(*(data_request->data), el, d) {
                                        XBT_CDEBUG(XKSIMU_DATAS, "     > Data Requested : %s", d->id);
                                        bool ok = false;

                                        struct xksimu_com_data_t * d1 = NULL;
                                        struct xksimu_list_el_t * el1 = NULL;
                                        xksimu_list_foreach(*(com->requests), el1, d1) {
                                                // Is the data (d) already requested
                                                if (d1->data == d) {
                                                        XBT_CDEBUG(XKSIMU_DATAS, "             > Data %s already requested : %p == %p (%s == %s)", d->id, d1->data, d, d1->data->id, d->id);
                                                        data = d1;
                                                        ok = true;
                                                        break;
                                                } else {
                                                        ok = false;
                                                }
                                        }

                                        // A new requested data
                                        if (!ok) {
                                                XBT_CDEBUG(XKSIMU_DATAS, "             > New requested data, creating com->request");
                                                data = calloc(1, sizeof(struct xksimu_com_data_t));
                                                data->requested = false;
                                                data->data = d;
                                                data->blocks = calloc(1, sizeof(struct xksimu_list_t));
                                                data->awaiting_blocks = calloc(1, sizeof(struct xksimu_list_t));
                                                data->applicants = calloc(1, sizeof(struct xksimu_list_t));
                                       
                                                data->blocks->head = NULL;
                                                data->blocks->queue= NULL;
                                                data->blocks->size = 0;

                                                struct xksimu_block_t * b;
                                                struct xksimu_list_el_t * el2;

                                                // Building the list of missings blocks
                                                xksimu_list_foreach(d->blocks, el2, b) {
                                                        if (! xksimu_list_contains(b->owners, host)) {
                                                                xksimu_list_push_back(data->blocks, b);
                                                        }
                                                }

                                                xksimu_list_push_back(com->requests, data);
                                                XBT_CDEBUG(XKSIMU_DATAS, "             > Added ! => %d com->requetes !", com->requests->size);
                                        }

                                        xksimu_list_push_back(data->applicants, applicant);
                                        xksimu_list_push_back(applicant->datas, data);
                                }
                                applicant->proc = data_request->source;

                                xksimu_list_push_back(com->applicants, applicant);

                                // The lists are completed, proceed to the demands
                                // We send a request only if it is a new data request
                                struct xksimu_com_data_t * d1;
                                struct xksimu_list_el_t * el1;
                                XBT_CDEBUG(XKSIMU_DATAS, "     > %d com->requetes !", com->requests->size);
                                int i = 0;
                                xksimu_list_foreach(*(com->requests), el1, d1) {
                                        i++;
                                        XBT_CDEBUG(XKSIMU_DATAS, "     > Current com->request - status : %d, data : %s", d1->requested, d1->data->id);
                                        if (!d1->requested) {
                                                d1->requested = true;
                                                for (int j = 0 ; j < 2 ; j++) {
                                                        struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                                        struct xksimu_com_request_data_t * request = calloc(1, sizeof(struct xksimu_com_request_data_t));
                                
                                                        // Choosing a random host
                                                        struct xksimu_host_t * host_dest = NULL;
                                                        do {
                                                                host_dest = &(hosts_table[rand_n(nb_hosts -1)]);
                                                        } while (host_dest->id == host->id);
                                
                                                        com_request->type = XKS_COM_REQ_DATA_DISTANT;
                                                        com_request->data = request;
                                                        request->source = host;
                                                        request->data = d1->data;
                                                        request->blocks = d1->blocks;

                                                        XBT_CINFO(XKSIMU_DATAS, "[Request] Data request sent to %s for data %s v%d.", host_dest->name, d1->data->id, d1->data->version);
                                                        msg_task_t message_s = MSG_task_create(NULL, 100, 100, com_request);
                                                        MSG_task_set_category(message_s, "DATA");
                                                        MSG_task_isend(message_s, host_dest->name);
                                                        XBT_CDEBUG(XKSIMU_DATAS, "             > Sent now, next : %p", el1->next);
                                                }
                                        } else {
                                                XBT_CDEBUG(XKSIMU_DATAS, "             > Already sent, next : %p", el1->next);
                                        }
                                }
                                
                                free(data_request);
                                break;
                        } case XKS_COM_REQ_DATA_DISTANT: {
                                struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                struct xksimu_com_request_data_answer_t * answer = calloc(1, sizeof(struct xksimu_com_request_data_answer_t));
                                struct xksimu_com_request_data_t * data_request = com_receive->data;

                                // XBT_INFO("Receive request for data %p (%d) from %s.", data_request->data, data_request->data->id, data_request->source->name);

                                answer->blocks = calloc(1, sizeof(struct xksimu_list_t));

                                answer->blocks->head = NULL;
                                answer->blocks->queue = NULL;
                                answer->blocks->size = 0;

                                // TODO : retirer ce hack tout moche (problème : head et queue != et != de null mais liste vide, ->size correct
                                if (data_request->blocks->size > 0) { 
                                // Compute the list of available blocks
                                struct xksimu_block_t * b;
                                struct xksimu_list_el_t * el;
                                xksimu_list_foreach (*(data_request->blocks), el, b) {
                                        if (xksimu_list_contains(b->owners, host)) {
                                                xksimu_list_push_back(answer->blocks, b);
                                        }
                                }
                                }

                                com_request->type = XKS_COM_REQ_DATA_ANSWER;
                                com_request->data = answer;
                                answer->source = host;
                                answer->data = data_request->data;

                                msg_task_t message_s = MSG_task_create(NULL, 1, 100, com_request);
                                MSG_task_set_category(message_s, "DATA");
                                MSG_task_isend(message_s, data_request->source->name);

                                if (answer->blocks->size != 0) {
                                        XBT_CDEBUG(XKSIMU_DATAS, "[Answer] %d blocks available for data %s.", answer->blocks->size, answer->data->id);
                                 } else {
                                        XBT_CDEBUG(XKSIMU_DATAS, "[Answer] No blocks available for %s.", data_request->source->name);
                                 }

                                free(data_request);
                                break;
                        } case XKS_COM_REQ_DATA_ANSWER: {
                                struct xksimu_com_request_data_answer_t * answer = com_receive->data;
                                struct xksimu_com_data_t * com_data = get_com_data(host, answer->data);

                                // The data could be already available (and so th com_data may not existe anymore) because there is to requests at the same time
                                if (com_data != NULL) {
                                        bool ok = false;

                                        if (answer->blocks->size != 0) {
                                                struct xksimu_block_t * block = get_first_intersection(com_data->blocks, answer->blocks);
                                                if (block != NULL) {
                                                        XBT_CINFO(XKSIMU_DATAS, "[Receive] %s have some available blocks for data %s v%d.", answer->source->name, answer->data->id, answer->data->version);
                                                        xksimu_list_remove(com_data->blocks, block);
                                                        xksimu_list_push_back(com_data->awaiting_blocks, block);
                                                        
                                                        struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                                        struct xksimu_com_request_block_t * request = calloc(1, sizeof(struct xksimu_com_request_block_t));

                                                        com_request->type = XKS_COM_REQ_BLOCK_DISTANT;
                                                        com_request->data = request;
                                                        request->block = block;
                                                        request->source = host;

                                                        msg_task_t message_s = MSG_task_create(NULL, 100, 100, com_request);
                                                        MSG_task_set_category(message_s, "DATA");
                                                        MSG_task_isend(message_s, answer->source->name);
                                                        XBT_CINFO(XKSIMU_DATAS, "[Request] Block request sent to %s for data %s v%d/%d.", answer->source->name, answer->data->id, answer->data->version, request->block->id);
                                                        ok = true;
                                                }
                                        }
                                        
                                        if (com_data->blocks->size > 0 && !ok) { 
                                                // The previous host doesn't have any blocks for us and we still need another ones, so we send a new request

                                                XBT_CINFO(XKSIMU_DATAS, "[Receive] No blocks available in %s for data %s v%d.", answer->source->name, answer->data->id, answer->data->version);
                                                struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                                struct xksimu_com_request_data_t * request = calloc(1, sizeof(struct xksimu_com_request_data_t));
                                                struct xksimu_host_t * host_dest = NULL;
                                                do {
                                                        host_dest = &(hosts_table[rand_n(nb_hosts -1)]);
                                                } while (host_dest->id == host->id);
                                                
                                                com_request->type = XKS_COM_REQ_DATA_DISTANT;
                                                com_request->data = request;
                                                request->source = host;
                                                request->data = answer->data;
                                                request->blocks =  com_data->blocks;

                                                msg_task_t message_s = MSG_task_create(NULL, 100, 100, com_request);
                                                MSG_task_set_category(message_s, "DATA");
                                                MSG_task_isend(message_s, host_dest->name);
                                                XBT_CINFO(XKSIMU_DATAS, "[Request] Data request sent to %s for data %s v%d.", host_dest->name, answer->data->id, answer->data->version);
                                        }
                                }

                                free(answer->blocks);
                                free(answer);
                                break;
                        } case XKS_COM_REQ_BLOCK_DISTANT: {
                                struct xksimu_com_request_block_t * req = com_receive->data;
                                struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                struct xksimu_com_request_block_t * answer = calloc(1, sizeof(struct xksimu_com_request_block_t));
                                
                                XBT_CINFO(XKSIMU_DATAS, "[Receive] Block %s v%d/%d requested by %s.", req->block->data->id, req->block->data->version, req->block->id, req->source->name);

                                com_request->type = XKS_COM_REQ_BLOCK_ANSWER;
                                com_request->data = answer;
                                answer->source = host;
                                answer->block = req->block;

                                msg_task_t message_s = MSG_task_create(NULL, 100, answer->block->size, com_request);
                                MSG_task_set_category(message_s, "DATA");
                                MSG_task_isend(message_s, req->source->name);
                                
                                XBT_CINFO(XKSIMU_DATAS, "[Answer] Block %s v%d/%d sent to %s.", req->block->data->id, req->block->data->version, req->block->id, req->source->name);
                                free(req);
                                break;
                        } case XKS_COM_REQ_BLOCK_ANSWER: {
                                struct xksimu_com_request_block_t * answer = com_receive->data;
                                struct xksimu_com_data_t * com_data = get_com_data(host, answer->block->data);
                               
                                XBT_CINFO(XKSIMU_DATAS, "[Receive] Receive block %s v%d/%d from %s.", answer->block->data->id, answer->block->data->version, answer->block->id, answer->source->name);

                                // Update the block's owners
                                xksimu_list_push_back(&(host->data_blocks), answer->block);
                                xksimu_list_push_back(&(answer->block->owners), host);

                                // Remove the block's from the waiting list
                                xksimu_list_remove(com_data->awaiting_blocks, answer->block);

                                // Is the data complete ? If not, we have to send a new Data Request
                                if (com_data->blocks->size != 0) {
                                        struct xksimu_com_request_t * com_request = calloc(1, sizeof(struct xksimu_com_request_t));
                                        struct xksimu_com_request_data_t * request = calloc(1, sizeof(struct xksimu_com_request_data_t));

                                        // To choose a new randoms host (can be the same as before
                                        struct xksimu_host_t * host_dest = NULL;
                                        do {
                                                host_dest = &(hosts_table[rand_n(nb_hosts -1)]);
                                        } while (host_dest->id == host->id);
                                
                                        com_request->type = XKS_COM_REQ_DATA_DISTANT;
                                        com_request->data = request;
                                        request->source = host;
                                        request->data = com_data->data;
                                        request->blocks = com_data->blocks;

                                        XBT_CINFO(XKSIMU_DATAS, "[Request] Data request sent to %s for data %s v%d. %d blocks missing", host_dest->name, com_data->data->id, com_data->data->version, com_data->blocks->size);
                                        msg_task_t message_s = MSG_task_create(NULL, 100, 100, com_request);
                                        MSG_task_set_category(message_s, "DATA");
                                        MSG_task_isend(message_s, host_dest->name);

                                } else if (com_data->awaiting_blocks->size == 0) {
                                        // We don't need to require another block and we receive the last one. So the data is now fully awailable

                                        XBT_CINFO(XKSIMU_DATAS, "Data %s fully available now.", com_data->data->id);
                                        
                                        // The applicants are suspended, we have to wake up them if they don't need any other data
                                        // And then to free the structures
                                        struct xksimu_com_applicant_t * a;
                                        struct xksimu_list_el_t * el2;
                                        xksimu_list_foreach(*(com->applicants), el2, a) {
                                                xksimu_list_remove(a->datas, com_data);

                                                if(a->datas->size == 0) {
                                                        // TODO : Compute the list of endeed applicants and remove it from com->applicants
                                                        XBT_INFO("Resume process %p (%d)", a->proc->process, a->proc->pid);
                                                        MSG_process_resume(a->proc->process);
                                                }
                                        }
                                        
                                        // We don't need this data anymore
                                        void * __res = xksimu_list_remove(com->requests, com_data);
                                        XBT_CDEBUG(XKSIMU_DATAS, "     > Remove data %s from com->requests. => %p for %p.", com_data->data->id, __res, com_data);
                                        xksimu_list_free(com_data->applicants);
                                        free(com_data->blocks);
                                        free(com_data->applicants);
                                        com_data->applicants = NULL;
                                        com_data->blocks = NULL;
                                        free(com_data);
                                        com_data = NULL;
                                } else {
                                        // We don't need another block but we are still waiting one or more of them from a remote so we can't do anything else for now
                                }
                                
                                free(answer);
                                break;
                        } default: {
                                XBT_WARN("CASE %d unknow.", com_receive->type);
                        }
                }
                free(com_receive);
        }

        return 0;
}

// ----- MAIN FUNCTION -----

int main (int argc, char *argv[]) {
        msg_error_t res = MSG_OK;
        const char * platform_file;
        const char * application_file;
        
        xbt_log_control_set("XKSIMU.thresh:INFO");
        xbt_log_control_set("XKSIMU_INIT.thresh:WARNING");

        MSG_init(&argc, argv);
        if (argc < 5) {
                printf("Usage: %s platform_file deployment_file tasks_file datas_file[blocks size]\n", argv[0]);
                printf("example: %s msg_platform.xml msg_deployment.xml graph.tasks graph.data 15000\n", argv[0]);
                exit(1);
        }
        platform_file = argv[1];
        application_file = argv[2];

        { /* Simulation setting */
                MSG_create_environment(platform_file);
        }
        { /* Application deployment */
                MSG_function_register("worker", worker);
                MSG_function_register_default(worker);
                MSG_function_register("comm", comm);
                MSG_launch_application(application_file);
        }

        if (init_simulation(argc - 3, &argv[3]) != 0) {
                exit(1);
        }

        srand(time(NULL));

        TRACE_platform_graph_export_graphviz("test_graph.dot");
        TRACE_category("TASK");
        TRACE_category("DATA");
        TRACE_category("LOCAL");

        XBT_INFO("Simulation started.");
        res = MSG_main();
        XBT_INFO("Simulation time %g", MSG_get_clock());
        if (res == MSG_OK) {
                return 0;
        } else {
                return 1;
        }
}
