#include "ble.h"

TaskHandle_t ble_setup_task_handler;
static btstack_context_callback_registration_t cmd_callback_registration;
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size);
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

const uint8_t adv_data[] = {
    // Flags general discoverable, BR/EDR not supported
    0x02, 0x01, 0x06, 
    // Name
    0x08, 0x09, 'B', 'l', 'u', 'e', 'P', 'a', 'd', 
};
const uint8_t adv_data_len = sizeof(adv_data);

OnReadCallback onReadCallback = nullptr;
OnWriteCallback onWriteCallback = nullptr;

void ble_setup(OnReadCallback onRead, OnWriteCallback onWrite) {
  onReadCallback = onRead;
  onWriteCallback = onWrite;

  xTaskCreatePinnedToCore(
      ble_setup_task, /* Function to implement the task */
      "Task1", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &ble_setup_task_handler,  /* Task handle. */
      0 /* Core where the task should run */
    );
}

void ble_setup_task(void * parameter) {
  printf("Setting up ATT Server");
  printf("On core: %d\n", xPortGetCoreID());

  att_server_init(profile_data, att_read_callback, att_write_callback);    
  
  // setup advertisements
  uint16_t adv_int_min = 0x0030;
  uint16_t adv_int_max = 0x0030;
  uint8_t adv_type = 0;
  bd_addr_t null_addr;
  memset(null_addr, 0, 6);
  gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);
  gap_advertisements_set_data(adv_data_len, (uint8_t*) adv_data);
  gap_advertisements_enable(1);

  vTaskDelete(ble_setup_task_handler);
}

static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size){
  UNUSED(connection_handle);

  if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE){
    const char * response = onReadCallback();
    
    return att_read_callback_handle_blob((const uint8_t *)response, (uint16_t) strlen(response), offset, buffer, buffer_size);
  }

  return 0;
}

static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size){
  UNUSED(transaction_mode);
  UNUSED(offset);
  UNUSED(buffer_size);
  UNUSED(connection_handle);    
  
  if (att_handle == ATT_CHARACTERISTIC_0000FF11_0000_1000_8000_00805F9B34FB_01_VALUE_HANDLE) {
    cmd_callback_registration.callback = &Task2Code;
    cmd_callback_registration.context = (void *)buffer;
    btstack_run_loop_execute_on_main_thread(&cmd_callback_registration);
  }

  return 0;
}

static void Task2Code(void* context) {
  uint8_t* ctx = (uint8_t*)context;

  onWriteCallback(ctx);
}
