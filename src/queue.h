#ifndef IOT_SYSTEM_SIM_QUEUE_H
#define IOT_SYSTEM_SIM_QUEUE_H

typedef struct task {
    char *type; //task type, either user type or sensor type
    int priority; //task priority
    char *ID; //task ID
    char *com_key; //task command if user-task or task key if sensor-task
    int args_num;
    char **args_vals; //task arguments if user-task or task values if sensor-task
    struct task* next;
}Task;

/* Set the queue size variable from memory provided by the config file */
void set_queue_size();

/* Creates and pushes a task with provided attributes to the provided queue
 * Returns 1 if successful, 0 otherwise */
int push(Task **head, char *type, int priority, char *ID, char *com_key, int args_num, char **args_vals);

/* Pops the first/head task in the provided queue
 * Returns 1 if successful, 0 otherwise */
int pop(Task **head);

/* Get the first/head task in the provided queue */
Task get(Task **head);

/* Outputs a given task to the stdin and log file */
void outputTask(Task **head);

/* Returns 1 if provided queue is empty, 0 otherwise */
int isEmpty(Task **head);

/* Returns 1 if provided queue is full, 0 otherwise */
int isFull(Task **head);

#endif //IOT_SYSTEM_SIM_QUEUE_H
