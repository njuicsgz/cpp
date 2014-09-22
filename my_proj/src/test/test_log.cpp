#include "Log.h"
#include <stdlib.h>
#include <unistd.h>

void * Process_thread(void *arg)
{
    for(;;)
    {
        Info("thread %u: test info", (unsigned int)pthread_self());
        usleep(0);
    }
}

int main(int argc, char **argv)
{
    G_pLog = CreateLog("test.log", 3, 1024, 0);

    pthread_t tid;
    for(int g_id = 0; g_id < atoi(argv[1]); g_id++)
    {
        if (pthread_create(&tid, NULL, Process_thread, NULL) != 0)
        {
            Err("%s","[Main]:Error on pthread_create");
            return -1;
        }
    }

    sleep(3*24*60*60);

    return 0;
}
