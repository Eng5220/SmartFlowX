import lgpio
import time

h = lgpio.gpiochip_open(4)
lgpio.gpio_claim_output(h, 17)

print("Writing LOW (0) - active-LOW relay should CLICK and open valve...")
lgpio.gpio_write(h, 17, 0)
time.sleep(3)

print("Writing HIGH (1) - relay should de-energize and close valve...")
lgpio.gpio_write(h, 17, 1)
time.sleep(3)

lgpio.gpiochip_close(h)
print("Done. Did you hear two clicks?")
