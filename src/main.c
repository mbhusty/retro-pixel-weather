#include <pebble.h>
  
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define KEY_NAME 0
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static BitmapLayer *s_icon_layer;
static GBitmap *s_icon_bitmap = NULL;
 
static GFont s_time_font;
static GFont s_weather_font;
 
static AppSync s_sync;
static uint8_t s_sync_buffer[64];

static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;

 static const uint32_t WEATHER_ICONS[] = {
  RESOURCE_ID_IMAGE_SUN, // 0
  RESOURCE_ID_IMAGE_CLOUD, // 1
  RESOURCE_ID_IMAGE_RAIN, // 2
  RESOURCE_ID_IMAGE_SNOW // 3
};

static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

enum WeatherKey {
  WEATHER_ICON_KEY = 0x0,         // TUPLE_INT
};

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case WEATHER_ICON_KEY:
      if (s_icon_bitmap) {
        gbitmap_destroy(s_icon_bitmap);
      }

      s_icon_bitmap = gbitmap_create_with_resource(WEATHER_ICONS[new_tuple->value->uint8]);

#ifdef PBL_SDK_3
      bitmap_layer_set_compositing_mode(s_icon_layer, GCompOpSet);
#endif

      bitmap_layer_set_bitmap(s_icon_layer, s_icon_bitmap);
      break;
   
  }
}



static void update_time() {
  // Получаем время и посылаем на часы
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
 
  // Create a long-lived buffer
  static char buffer[] = "00:00";
 
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 2h hour format
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
  } else {
    //Use 12 hour format
    strftime(buffer, sizeof("00:00"), "%I:%M", tick_time);
  }
 
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, buffer);
}

 
static void main_window_load(Window *window) {
  
 
//   //Create GBitmap, then set to created BitmapLayer
// s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BACK_GROUND);
// s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
// bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
// layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
  
   
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, 0, 144, 168));
  text_layer_set_background_color(s_time_layer, GColorBlack);
  text_layer_set_text_color(s_time_layer, GColorWhite);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  text_layer_set_text(s_time_layer, "00:00");
  
  //Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MINECRAFT_34));
 
  //Apply to TextLayer
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
 
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  
      // Create temperature Layer
  s_weather_layer = text_layer_create(GRect(0, 125, 144, 168));
  text_layer_set_background_color(s_weather_layer, GColorBlack);
  text_layer_set_text_color(s_weather_layer, GColorWhite);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Wait plz...");
   //layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  
 
  
    s_icon_layer = bitmap_layer_create(GRect(32, 45, 80, 80));
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_icon_layer));
  
  
  

  
  
  // Create second custom font, apply it and add to Window
  s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_MINECRAFT_26));
  text_layer_set_font(s_weather_layer, s_weather_font);
 
  
  
    Tuplet initial_values[] = {
    TupletInteger(WEATHER_ICON_KEY, (uint8_t) 1),
    
  };

  app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
      initial_values, ARRAY_LENGTH(initial_values),
      sync_tuple_changed_callback, sync_error_callback, NULL);

   
  // Make sure the time is displayed from the start
  update_time();
}
 
static void main_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  
  //Destroy GBitmap
  gbitmap_destroy(s_background_bitmap);
 
  //Destroy BitmapLayer
  bitmap_layer_destroy(s_background_layer);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  
  // Destroy weather elements
  text_layer_destroy(s_weather_layer);
  fonts_unload_custom_font(s_weather_font);
}
 
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 10 minutes
  if(tick_time->tm_min % 15 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
 
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
 
    // Send the message!
    app_message_outbox_send();
  }
}
 
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);
 
  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
      break;
//     case KEY_CONDITIONS:
//       snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
//       break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }
 
    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s%s", temperature_buffer, conditions_buffer);
  text_layer_set_text(s_weather_layer, weather_layer_buffer);
}
 
static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}
 
static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}
 
static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();
 
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
 
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}
 
static void deinit() {
  // Destroy Window
  if (s_icon_bitmap) {
    gbitmap_destroy(s_icon_bitmap);
  }
  window_destroy(s_main_window);
  bitmap_layer_destroy(s_icon_layer);
  
}
 
int main(void) {
  init();
  app_event_loop();
  deinit();
}