# Tester

Hardware tester script for ARDEP boards. Tests `GPIO`, `LIN`, `CAN` and `UART`.

Connect the 2 ARDEP boards with each other. Ensure that all pins mentioned in the board overlays for the `hardware_test(er)` are connected between the two boards where GPIO pins and devices with the same name are connected to one another. E.g. connect `CAN_A` on the tester board with `CAN_A` on the SUT.

Ensure, that the SUT runs the `hardware_test` firmware and the tester runs the `hardware_tester` firmware.

To run the test, run the tester script e.g. with

```
python3 ./scripts/tester/tester.py /dev/ttyACM0 /dev/ttyACM1;
```


# BEWARE !!

Currently the tests are flaky. Execute the test sereval times to be sure. 

e.g.
```sh
for i in {1..4}; do ./scripts/tester/tester.py /dev/ttyACM0 /dev/ttyACM1 ; done
```