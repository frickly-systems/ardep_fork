ARDEP - Automotive Rapid Development Platform
##############################################

..  image:: boards/mercedes/ardep/doc/3d_view_v2.png
    :align: center
    :width: 50%


ARDEP (**A**\ utomotive **R**\ apid **DE**\ velopment **P**\ latform) is a powerful toolkit, consisting of the ARDEP Board and a software stack, specifically designed for automotive developers based on the `Zephyr RTOS <https://www.zephyrproject.org/>`_.
It provides easy to use abstractions, features and tools to simplify the development process for automotive applications:

* **Development board**: The integrated development board allows you to quickly prototype and test your ideas, providing easy access to communication interfaces and GPIO's.

  * **No need for external transceivers**: Onboard CAN and LIN transceivers give you the output you really want.
  * **Versatile connectivity**: CAN, LIN, SPI, I2C, UART and more are all supported by ARDEP with easy to use APIs, enabling seamless integration with other automotive systems.

* **Robust framework**: The ARDEP provides a solid foundation for building automotive applications.

  * **Zephyr RTOS based**: Built on the reliable and widely-used Zephyr RTOS, ensuring stability and performance, with access to a large ecosystem of libraries, tools and drivers.
  * **UDS support**: Built-in support for different UDS Services with easy to use APIs
  * **DFU over UDS**: Easily update your firmware over UDS with the integrated DFU functionality.

* **Automotive-focused**: The project is specifically tailored to the needs of automotive developers, ensuring compatibility with industry protocols.
* **Open Source**: From Hardware to Software, ARDEP is fully open source, licensed under the Apache 2.0 license


Getting Started
===============

See our `documentation <https://mercedes-benz.github.io/ardep/>`_  for more information on how to use ARDEP.

Follow our `Getting Started Guide <https://mercedes-benz.github.io/ardep/getting_started/index.html>`_ for a quick introduction


Quick Start
===========

Create workspace from west.yml in this directory, e.g.


.. code-block:: console

    # create a workspace
    mkdir ardep-workspace
    # clone this repo into workspace
    cd ardep-workspace && git clone git@github.com:mercedes-benz/ardep.git  ardep
    # init west workspace from west.yml
    cd ardep && west init -l --mf ./west.yml .
    # update workspace, fetches dependencies
    west update


Contributing
============

Contributions are welcome! Please see `CONTRIBUTING.md <CONTRIBUTING.md>`_ for more information.


.. _license:
License
=======
Copyright Mercedes-Benz AG

This project is licensed under the `Apache 2.0 license <LICENSE>`_.
