# ocaml-riot-nrf52

This project gets OCaml bytecode running on the nRF52840 board. It uses the standard OCaml runtime, and RIOT OS for the base layer between the runtime and the board, as well as for its robust build system.

This is the previous version of [**omicrob-riot-nrf52**](https://github.com/benmandrew/omicrob-riot-nrf52 "omicrob-riot-nrf52"), which uses the OMicroB runtime instead of the standard OCaml one, as well as OMicroB's bytecode optimisations. This project runs into issues of large images as well as issues with dynamic memory allocation that prevent the effective use of OCaml modules. It is kept public as it is still useful to learn from.

## Building

```
git clone https://github.com/benmandrew/ocaml-riot-nrf52.git
cd ocaml-riot-nrf52/ocaml-nrf52
./configure.sh
make
```
