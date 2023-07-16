# ArduPlot

A real-time serial plotter that automatically adjusts itself to your use case.

![ArduPlot v0.2a](img/ArduPlot%20v0.2a_1.png)

ArduPlot acts both as a traditional serial monitor and a real-time data visualization tool.

When a specially formatted packet is received by ArduPlot, such as this one:

``` json
{"1&n&l&Gyro X":-1.892217994,"1&n&l&Gyro Y":-3.09271574,"1&n&l&Gyro Z":-0.000549316,"2&n&l&Front distance (cm)":0,"2&n&l&Front strength":0,"0&n&h&VL53L5LX&8&8&0&1000":[662,653,644,643,639,637,635,595,648,648,633,634,633,632,621,604,652,637,633,634,628,619,593,405,631,637,641,607,622,598,311,283,326,320,327,315,299,290,284,272,276,278,276,282,283,294,314,296,274,272,275,266,277,299,322,302,218,223,207,185,203,231,245,261],"3&n&l&Color: b_comp":0,"3&n&l&Color: c_comp":0,"3&n&l&Color: g_comp":0,"3&n&l&Color: r_comp":0,"0&i":3}
```

It gets parsed and turned into different graphs, depending on the json key value.

This leaves to the microcontroller the choice of how to best display the data it has.

To correctly format the data, [MicroPlot](https://github.com/Dave879/MicroPlot) is used.

## Supported graph types

- Line
- Multi-line
- Heatmap

## Future features

- Bar graph
- Scatter plot
- Scroll back and stop ArduPlot time
- Save and inspect data
- Optional CRC in communication

![ArduPlot v0.3a](img/ArduPlot%20v0.3a_1.png)

![ArduPlot v0.1a](img/ArduPlot%20v0.1a_1.png)
