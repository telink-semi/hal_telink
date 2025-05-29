
#include"service/service_mailbox.h"
#include"service/service_shareMemory.h"

void tlk_multi_core_communication_init(void)
{
    tlk_mailbox_service_init();

    tlk_share_memory_service_init();
}

void tlk_multi_core_communication_loop(void)
{
    tlk_mailbox_service_loop();

    tlk_share_memory_service_loop();
}
