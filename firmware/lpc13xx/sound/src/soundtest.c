#include <openbeacon.h>

/* 8xLED bar display */
void
set_led (uint8_t led)
{
  LPC_GPIO2->DATA = (LPC_GPIO2->DATA & ~0xF0) | (~led & 0xF0);
  LPC_GPIO3->DATA = (LPC_GPIO3->DATA & ~0x0F) | (~led & 0x0F);
}

int
main (void)
{
  volatile int t;
  int i;

  /* Initialize GPIO (sets up clock) */
  GPIOInit ();
  GPIOSetDir (BUT1_PORT, BUT1_PIN, 0);
  GPIOSetDir (BUT2_PORT, BUT2_PIN, 0);

  /* Set LED port pin to output */
  LPC_GPIO2->DIR |= 0xF0;
  LPC_GPIO3->DIR |= 0x0F;

  i = 0;
  while (1)
    {
      /* BUTTON1 press cycles through tones */
      if (!GPIOGetValue (BUT1_PORT, BUT1_PIN))
	{
	  i = (i + 1) & 0x7;

	  for(t=0;t<1000000;t++);
	}

      /* BUTTON2 plays tone */
      if (!GPIOGetValue (BUT2_PORT, BUT2_PIN))
	{
	  set_led (0xFF);

	  for(t=0;t<1000000;t++);
	}

      set_led (1 << i);
    }
}
