# Embedded system project (Fall semester, 2020)

## objective
To build pet-caring robot with raspi

## Important Notes
1. For bluedot app, following file should be revised to work:
At /usr/local/lib/python3.7/dist-packages/bluedot/dot.py, line 1206(or nearby), should be revised to:
```
def resize(self, cols, rows):
        """
        Resizes the grid of buttons. 

        :param int cols:
            The number of columns in the grid of buttons.

        :param int rows:
            The number of rows in the grid of buttons.

        .. note::
            Existing buttons will retain their state (color, border, etc) when 
            resized. New buttons will be created with the default values set 
            by the :class:`BlueDot`.
        """
        self._cols = cols
        self._rows = rows        

        # create new buttons
        new_buttons = {}
        

        # Rewrite Code Version Here
        # @ by Dongmin Kim
        interaction = None
        for c in range(cols):
            for r in range(rows):
                # Get config from prev button
                if (c,r) in self._buttons.keys():
                    interaction = self._buttons[c,r]._interaction
                new_buttons[c,r] = BlueDotButton(self, c, r, self._color, self._square, self._border, self._visible)
                new_buttons[c,r]._interaction = interaction

        # Original Version
        '''
        for c in range(cols):
            for r in range(rows):
                # if button already exist, reuse it
                if (c,r) in self._buttons.keys():
                    new_buttons[c,r] = self._buttons[c,r]
                else:
                    new_buttons[c,r] = BlueDotButton(self, c, r, self._color, self._square, self._border, self._visible)
        '''
        self._buttons = new_buttons
        self._send_bluedot_config()

```

2. For i2c Controls to work(display, humidity sensor):
At boot/config.txt
```
dtparam=i2c_arm=on,i2c_arm_baudrate=200000
```

3. Crontabs for scheduled jobs
```
$ sudo crontab -e

@reboot sudo sh /home/pi/launcher.sh > /home/pi/logs/cronlog 2>&1
*/1 * * * * /home/pi/modules/sensors/climate

```


## Progress

### 2020-12-10: dancing with music
- [dance](./modules/dance.py)

### 2020-11-15: motor control
- Our first drive: [code for motor](./modules/motor.py)  
  
  ![motor1](./imgs/motor.gif)

### 2020-11-07: i2c control
- display setups: [code for display](./i2c/display_main.c)
  
  ![display](./imgs/display.gif)

### 2020-10-07: gpio control
- LED Light on: [code for gpio light bulb](./gpio_control/gpio.c)
  
  ![light](./imgs/light.gif)

### 2020-09-21: cross compile
[process for cross compile](https://dongminkim0220.github.io/posts/cross-compile/)
