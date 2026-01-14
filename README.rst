ARDEP - Automotive Rapid Development Platform
#############################################

.. image:: boards/mercedes/ardep/doc/3d_view_v2.png
   :align: center
   :width: 50%


ARDEP (**A**\ utomotive **R**\ apid **DE**\ velopment **P**\ latform) is an open-source
hardware and software platform designed for rapid prototyping and development
of automotive products based on the `Zephyr RTOS <https://www.zephyrproject.org/>`_.

It targets automotive developers and alike who need a robust, UDS-capable development environment with
real automotive communication interfaces.


Key Features
============

* **Automotive development board**

  * Open-Source Integrated development board for rapid prototyping and evaluation
  * Onboard CAN and LIN transceivers: no external transceivers required
  * Onboard debugger and programmer with UART connection
  * Easy access to GPIOs and communication interfaces

* **PowerIO Shield**

  * Dedicated PowerIO Shield for extended power capabilities
  * Six high-power high-side switches with up to 3A continuous current per channel and 10A in total at up to 48V
  * Six 48V tolerant digital inputs
  * Stackable design for up to 8 PowerIO shields
  * Abstraction layer for easy control of high-voltage IOs with Zephyr's GPIO API

* **Versatile connectivity**

  * CAN, LIN, SPI, I2C, UART and more
  * Unified and easy-to-use APIs for automotive communication

* **Robust software framework**

  * Solid foundation for building automotive applications
  * Designed for modularity and long-term maintainability

* **Automotive diagnostics**

  * Built-in support for UDS services (ISO 14229)
  * Integrated DFU over UDS for firmware updates
  * Configurable UDS addresses via Jumpers on the board

* **Zephyr RTOS based**

  * Built on the reliable and widely-used Zephyr RTOS
  * Access to a large ecosystem of drivers, tools and libraries


Getting Started
===============

Please refer to the `documentation <https://mercedes-benz.github.io/ardep/>`_
for detailed information on how to use ARDEP.

A step-by-step introduction is available in the
`Getting Started Guide <https://mercedes-benz.github.io/ardep/getting_started/index.html>`_.


Quick Start
===========

Create a Zephyr workspace using the provided ``west.yml`` file.

.. code-block:: console

    # create a workspace
    mkdir ardep-workspace

    # clone this repository into the workspace
    cd ardep-workspace && git clone git@github.com:mercedes-benz/ardep.git ardep

    # initialize the west workspace
    cd ardep && west init -l --mf ./west.yml .

    # fetch all dependencies
    west update


Contributing
============

Contributions are welcome and appreciated.
Please see `CONTRIBUTING.md <CONTRIBUTING.md>`_ for guidelines and further
information.


License
=======

Copyright Mercedes-Benz AG

This project is licensed under the `Apache 2.0 License <LICENSE>`_.
