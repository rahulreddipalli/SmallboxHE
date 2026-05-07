#include "input_events.h"

#define INPUT_EVENTS_QUEUE_SIZE 8U

static UiEvent event_queue[INPUT_EVENTS_QUEUE_SIZE];
static uint8_t event_head;
static uint8_t event_tail;
static uint8_t event_count;

void InputEvents_Init(void)
{
  event_head = 0U;
  event_tail = 0U;
  event_count = 0U;
}

void InputEvents_Update(void)
{
  /*
   * Rotary hardware is not wired into firmware yet. Future polling or debounced
   * ISR handoff should enqueue events here without putting UI logic in an ISR.
   */
}

uint8_t InputEvents_Push(UiEvent event)
{
  if (event == UI_EVT_NONE)
  {
    return 0U;
  }

  if (event_count >= INPUT_EVENTS_QUEUE_SIZE)
  {
    return 0U;
  }

  event_queue[event_head] = event;
  event_head = (uint8_t)((event_head + 1U) % INPUT_EVENTS_QUEUE_SIZE);
  event_count++;

  return 1U;
}

UiEvent InputEvents_Pop(void)
{
  UiEvent event;

  if (event_count == 0U)
  {
    return UI_EVT_NONE;
  }

  event = event_queue[event_tail];
  event_tail = (uint8_t)((event_tail + 1U) % INPUT_EVENTS_QUEUE_SIZE);
  event_count--;

  return event;
}
