# RP2350 + LAN8651 Arduino Compilation Fix

Place `board.json` files in `boards/` and specify the path to RP2040POE in `platformio.ini` for variants.

Replace `PIN_SPIETH_*` in `pins_arduino.h` from variants with `PIN_SPI1_*`. (To use SPI1)

Current possibly problematic warnings:
```
.pio/libdeps/rp2350/Arduino_10BASE_T1S/src/MacAddress.cpp:100:3: warning: #warning "Retrieving a unique chip ID for MAC generation is not supported on this platform." [-Wcpp]
  100 | # warning "Retrieving a unique chip ID for MAC generation is not supported on this platform."
      |   ^~~~~~~
In file included from src/main.ino:3:
.pio/libdeps/rp2350/Arduino_10BASE_T1S/src/Arduino_10BASE_T1S.h:54:3: warning: #warning "No pins defined for your board" [-Wcpp]
   54 | # warning "No pins defined for your board"
      |   ^~~~~~~
```

Using [main.ino](src/main.ino) which is based on an example from the [Arduino_10BASE_T1S repository](https://github.com/arduino-libraries/Arduino_10BASE_T1S), PHY currently cannot be initialized. (Serial responded with `'TC6::begin(...)' failed.`) (Probably hasn't changed IRQ_PIN to PIN_SPI1_IRQ)

## platformio.ini

Uses [build_flags.py](build_flags.py) to suppress some errors.

Uses [Arduino_10BASE_T1S Fork](https://github.com/RadicalTray/Arduino_10BASE_T1S) which removes conflicting symbols and defines.

```ini
[env:rp2350]
platform = https://github.com/maxgerhardt/platform-raspberrypi.git
framework = arduino
board_build.core = earlephilhower

; specify the board
board = vaamrp2350_focspe
; path to the RP2040POE repository (variants)
board_build.variants_dir = ./variants
; specify the variant
board_build.variant = vaam_rp2350spefoc

extra_scripts = pre:build_flags.py
lib_deps =
	https://github.com/RadicalTray/Arduino_10BASE_T1S
```

## build_flags.py

Suppresses this error:
```
.pio/libdeps/rp2350/Arduino_10BASE_T1S/src/lib/liblwip/core/../include/lwip/../../cfg/lwipopts.h:617:19: error: implicit declaration of function 't1s_lwip_itoa'; did you mean 'lwip_itoa'? [-Wimplicit-function-declaration]
```

```py
Import("env")

env.Append(CFLAGS=[
    "-Wno-implicit-function-declaration", # Completely ignore the warning
    # "-Wno-error=implicit-function-declaration", # Keep the warning but don't error
])
```

## [Arduino_10BASE_T1S Fork](https://github.com/RadicalTray/Arduino_10BASE_T1S)

Don't define `lwip_htons` and `lwip_htonl` since those are already defined by FrameworkArduino. Fixes these errors:

```
.platformio/packages/toolchain-rp2040-earlephilhower/bin/../lib/gcc/arm-none-eabi/14.3.0/../../../../arm-none-eabi/bin/ld: .pio/build/rp2350/libFrameworkArduino.a(def.c.o): in function `lwip_htons':
def.c:(.text.lwip_htons+0x0): multiple definition of `lwip_htons'; .pio/build/rp2350/libe01/libArduino_10BASE_T1S-fork.a(def.c.o):def.c:(.text.lwip_htons+0x0): first defined here
.platformio/packages/toolchain-rp2040-earlephilhower/bin/../lib/gcc/arm-none-eabi/14.3.0/../../../../arm-none-eabi/bin/ld: .pio/build/rp2350/libFrameworkArduino.a(def.c.o): in function `lwip_htonl':
def.c:(.text.lwip_htonl+0x0): multiple definition of `lwip_htonl'; .pio/build/rp2350/libe01/libArduino_10BASE_T1S-fork.a(def.c.o):def.c:(.text.lwip_htonl+0x0): first defined here
```

```diff
diff --git a/src/lib/liblwip/core/def.c b/src/lib/liblwip/core/def.c
index ece1125..5003677 100644
--- a/src/lib/liblwip/core/def.c
+++ b/src/lib/liblwip/core/def.c
@@ -65,7 +65,8 @@
 
 #if BYTE_ORDER == LITTLE_ENDIAN
 
-#if !defined(lwip_htons)
+/* NOTE: Don't define this to not conflict with FrameworkArduino */
+#if 0
 /**
  * Convert an u16_t from host- to network byte order.
  *
@@ -79,7 +80,8 @@ lwip_htons(u16_t n)
 }
 #endif /* lwip_htons */
 
-#if !defined(lwip_htonl)
+/* NOTE: Don't define this to not conflict with FrameworkArduino */
+#if 0
 /**
  * Convert an u32_t from host- to network byte order.
  *
```

Ignore `cc.h` since symbols from it are already defined by framework-arduinopico. Suppresses redefine warnings.

>This patch is not needed to compile and could cause a problem. It is already included by [the Arduino_10BASE_T1S Fork](https://github.com/RadicalTray/Arduino_10BASE_T1S).

```diff
diff --git a/src/lib/liblwip/arch/cc.h b/src/lib/liblwip/arch/cc.h
index 4637e7d..f20fda0 100644
--- a/src/lib/liblwip/arch/cc.h
+++ b/src/lib/liblwip/arch/cc.h
@@ -31,6 +31,9 @@
  *
  */
 
+/* NOTE: Don't include this file because they are included by framework-arduinopico */
+#define CC_H_INCLUDED
+
 #ifndef CC_H_INCLUDED
 #define CC_H_INCLUDED
```

## Misc

Fixing `pins_arduino.h`.

```diff
diff --git a/vaam_rp2350spefoc/pins_arduino.h b/vaam_rp2350spefoc/pins_arduino.h
index 5f90e16..ec0db58 100644
--- a/vaam_rp2350spefoc/pins_arduino.h
+++ b/vaam_rp2350spefoc/pins_arduino.h
@@ -37,12 +37,12 @@
 #define PIN_SPI0_SS    (5u)
 
 // SPI ETH
-#define PIN_SPIETH_MISO  (16u)
-#define PIN_SPIETH_MOSI  (19u)
-#define PIN_SPIETH_SCK   (18u)
-#define PIN_SPIETH_SS    (17u)
-#define PIN_SPIETH_IRQ   (14u)
-#define PIN_SPIETH_RST    (15u)
+#define PIN_SPI1_MISO  (16u)
+#define PIN_SPI1_MOSI  (19u)
+#define PIN_SPI1_SCK   (18u)
+#define PIN_SPI1_SS    (17u)
+#define PIN_SPI1_IRQ   (14u)
+#define PIN_SPI1_RST    (15u)
 
 // Wire
 #define PIN_WIRE0_SDA  (10u)     // pico pin must be this pair
@@ -64,4 +64,4 @@
 
 
 
-#include "../generic/common.h"
\ No newline at end of file
+#include "../vaam_rp2040poe/common.h"
```
