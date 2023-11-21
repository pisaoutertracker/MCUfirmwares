import machine
from utime import sleep_ms, ticks_ms, ticks_diff

class ButtonHandler:
    def __init__(self, pin_number, callback, debounce_time=20):
        self.debounce_time = debounce_time
        self.callback = callback
        self.button = machine.Pin(pin_number, machine.Pin.IN, machine.Pin.PULL_UP)
        self.button.irq(trigger=machine.Pin.IRQ_FALLING, handler=self._irq_handler)
        self.previous_ticks = 0

    def _irq_handler(self, pin):
        current_ticks = ticks_ms()
        if ticks_diff(current_ticks, self.previous_ticks) > self.debounce_time:
            self.callback()
            self.previous_ticks = current_ticks

