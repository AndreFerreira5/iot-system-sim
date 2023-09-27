#include "queue.h"
#include "log.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int QUEUE_SZ;

void set_queue_size(){
    QUEUE_SZ = get_config_value("QUEUE_SZ");
}

void free_task_args(Task *temp){
    for(int i=0; i<temp->args_num; i++){
        free(temp->args_vals[i]);
    }
    free(temp->args_vals);
    free(temp->com_key);
    free(temp->ID);
    free(temp->type);
    free(temp);
}


//create and return a new task for the internal queue
Task* newTask(char *type, int priority, char *ID, char *com_key, int args_num, char **args_vals){

    if(!type || !ID || !com_key || args_num < 0){
        return NULL;
    }

    Task *temp = malloc(sizeof(Task));
    if(!temp){
        return NULL;
    }

    temp->type = strdup(type);
    if(!temp->type){
        free(temp);
        return NULL;
    }

    temp->priority = priority;

    temp->ID = strdup(ID);
    if(!temp->ID){
        free(temp->type);
        free(temp);
        return NULL;
    }

    temp->com_key = strdup(com_key);
    if(!temp->com_key){
        free(temp->ID);
        free(temp->type);
        free(temp);
        return NULL;
    }

    temp->args_vals = malloc(args_num * sizeof(char*));
    if(!temp->args_vals){
        free(temp->com_key);
        free(temp->ID);
        free(temp->type);
        free(temp);
        return NULL;
    }

    for(int i=0; i<args_num; i++){
        temp->args_vals[i] = strdup(args_vals[i]);
        if(!temp->args_vals[i]){
            free_task_args(temp);
            return NULL;
        }
    }

    temp->args_num = args_num;
    temp->next = NULL;

    return temp;
}

//output a given task to the stdin and log file
void outputTask(Task **head){
    Task *task = (*head);
    if(task){
        if (strcmp(task->type, "sensor") == 0){
            char *tmp_buffer = malloc(strlen("SENSOR TASK: ID: KEY: VALUE:") + strlen(task->ID) + strlen(task->com_key) + strlen(task->args_vals[0]) + 1);
            sprintf(tmp_buffer, "SENSOR TASK: ID:%s KEY:%s VALUE:%d", task->ID, task->com_key, atoi(task->args_vals[0]));
            request_log("INFO", tmp_buffer);
            free(tmp_buffer);
        }
        else if (strcmp(task->type, "user") == 0){
            if(task->args_num == 0){
                char *tmp_buffer = malloc(strlen("USER CONSOLE TASK: ID: COMMAND: ") + strlen(task->ID) + strlen(task->com_key) + 1);
                sprintf(tmp_buffer, "USER CONSOLE TASK: ID:%s COMMAND:%s ", task->ID, task->com_key);
                request_log("INFO", tmp_buffer);
                free(tmp_buffer);
            }
            else if(task->args_num == 1){
                char *tmp_buffer = malloc(strlen("USER CONSOLE TASK: ID: COMMAND: ARGUMENT:") + strlen(task->ID) + strlen(task->com_key) + strlen(task->args_vals[0]) + 1);
                sprintf(tmp_buffer, "USER CONSOLE TASK: ID:%s COMMAND:%s ARGUMENT:%s", task->ID, task->com_key, task->args_vals[0]);
                request_log("INFO", tmp_buffer);
                free(tmp_buffer);
            }
            else if(task->args_num == 2){
                char *tmp_buffer = malloc(strlen("USER CONSOLE TASK: ID: COMMAND: ARGUMENTS: ") + strlen(task->ID) + strlen(task->com_key) + strlen(task->args_vals[0]) + strlen(task->args_vals[1]) + 1);
                sprintf(tmp_buffer, "USER CONSOLE TASK: ID:%s COMMAND:%s ARGUMENTS:%s %s", task->ID, task->com_key, task->args_vals[0], task->args_vals[1]);
                request_log("INFO", tmp_buffer);
                free(tmp_buffer);
            }
            else if(task->args_num == 3){
                char *tmp_buffer = malloc(strlen("USER CONSOLE TASK: ID: COMMAND: ARGUMENTS:  ") + strlen(task->ID) + strlen(task->com_key) + strlen(task->args_vals[0]) + strlen(task->args_vals[1]) + strlen(task->args_vals[2]) + 1);
                sprintf(tmp_buffer, "USER CONSOLE TASK: ID:%s COMMAND:%s ARGUMENTS:%s %s %s", task->ID, task->com_key, task->args_vals[0], task->args_vals[1], task->args_vals[2]);
                request_log("INFO", tmp_buffer);
                free(tmp_buffer);
            }
            else if(task->args_num == 4){
                char *tmp_buffer = malloc(strlen("USER CONSOLE TASK: ID: COMMAND: ARGUMENTS:   ") + strlen(task->ID) + strlen(task->com_key) + strlen(task->args_vals[0]) + strlen(task->args_vals[1]) + strlen(task->args_vals[2]) + strlen(task->args_vals[3]) + 1);
                sprintf(tmp_buffer, "USER CONSOLE TASK: ID:%s COMMAND:%s ARGUMENTS:%s %s %s %s", task->ID, task->com_key, task->args_vals[0], task->args_vals[1], task->args_vals[2], task->args_vals[3]);
                request_log("INFO", tmp_buffer);
                free(tmp_buffer);
            }
        }
    }
}



//push a task to the INTERNAL QUEUE
int push(Task **head, char *type, int priority, char *ID, char *com_key, int args_num, char **args_vals){

    if(!type || !ID || !com_key || args_num < 0){
        return 0;
    }

    Task *new_task = newTask(type, priority, ID, com_key, args_num, args_vals);

    if(!new_task){
        return 0;
    }

    Task *current = *head;

    if(!*head || (*head)->priority > priority){
        new_task->next = *head;
        *head = new_task;
    }
    else{
        while(current->next && current->next->priority < priority){
            current = current->next;
        }
        new_task->next = current->next;
        current->next = new_task;
    }

    return 1;
}


//get the first task in INTERNAL QUEUE
Task get(Task **head){
    return *(*head);
}

int pop(Task **head){
    if(*head == NULL) return 0;

    Task *temp = *head;
    *head = (*head)->next;

    free(temp->type);
    free(temp->ID);
    free(temp->com_key);
    free(temp->args_vals);
    free(temp);

    return 1;
}

//returns 1 if INTERNAL QUEUE is empty, 0 otherwise
int isEmpty(Task **head) {
    return (*head) == NULL;
}

//returns 1 if INTERNAL QUEUE is full, 0 otherwise
int isFull(Task **head){
    Task *start = (*head);
    if((*head) != NULL) {
        int task_count = 0;
        while (start->next != NULL) { //iterate through the queue
            task_count++; //increment for each task
            start = start->next;
        }
        return task_count+1 >= QUEUE_SZ;
    }else{
        return 0;
    }
}