/************************************************************************/
/* XBoot Extensible AVR Bootloader                                      */
/*                                                                      */
/* tested with ATXMEGA64A3                                              */
/*                                                                      */
/* xboot.c                                                              */
/*                                                                      */
/* Alex Forencich <alex@alexforencich.com>                              */
/*                                                                      */
/* Copyright (c) 2010 Alex Forencich                                    */
/*                                                                      */
/* Permission is hereby granted, free of charge, to any person          */
/* obtaining a copy of this software and associated documentation       */
/* files(the "Software"), to deal in the Software without restriction,  */
/* including without limitation the rights to use, copy, modify, merge, */
/* publish, distribute, sublicense, and/or sell copies of the Software, */
/* and to permit persons to whom the Software is furnished to do so,    */
/* subject to the following conditions:                                 */
/*                                                                      */
/* The above copyright notice and this permission notice shall be       */
/* included in all copies or substantial portions of the Software.      */
/*                                                                      */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF   */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND                */
/* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS  */
/* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN   */
/* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN    */
/* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE     */
/* SOFTWARE.                                                            */
/*                                                                      */
/************************************************************************/

#include "xboot.h"

unsigned char comm_mode;

#ifdef USE_I2C
unsigned char first_byte;
#endif

int main(void)
{
        ADDR_T address = 0;
        unsigned char in_bootloader = 0;
        unsigned char val = 0;
        int i, j, k;
        void (*reset_vect)( void ) = 0x000000;
        
        #ifdef USE_I2C_ADDRESS_NEGOTIATION
        unsigned short devid_bit;
        #endif // USE_I2C_ADDRESS_NEGOTIATION
        
        comm_mode = MODE_UNDEF;
        
        // Initialization section
        // Entry point and communication methods are initialized here
        // --------------------------------------------------
        
        #ifdef USE_LED
        // Initialize LED pin
        LED_PORT.DIRSET = (1 << LED_PIN);
        #if LED_PIN_INV
        LED_PORT.OUTCLR = (1 << LED_PIN);
        #else
        LED_PORT.OUTSET = (1 << LED_PIN);
        #endif // LED_PIN_INV
        #endif // USE_LED
        
        #ifdef USE_I2C_ADDRESS_NEGOTIATION
        #ifdef USE_ATTACH_LED
        // Initialize ATTACH_LED
        ATTACH_LED_PORT.DIRSET = (1 << ATTACH_LED_PIN);
        #if ATTACH_LED_INV
        ATTACH_LED_PORT.OUTSET = (1 << ATTACH_LED_PIN);
        #else
        ATTACH_LED_PORT.OUTCLR = (1 << ATTACH_LED_PIN);
        #endif // ATTACH_LED_INV
        #endif // USE_ATTACH_LED
        #endif // USE_I2C_ADDRESS_NEGOTIATION
        
        #ifdef USE_ENTER_PIN
        // Make sure it's an input
        ENTER_PORT.DIRCLR = (1 << ENTER_PIN);
        #if ENTER_PIN_PUEN
        // Enable bootloader entry pin pullup
        ENTER_PIN_CTRL = 0x18;
        #endif
        #endif
        
        #ifdef USE_UART
        // Initialize UART
        #ifdef __AVR_XMEGA__
        UART_PORT.DIRSET |= UART_TX_PIN;
        UART_DEVICE.BAUDCTRLA = (UART_BSEL_VALUE & USART_BSEL_gm);
        UART_DEVICE.BAUDCTRLB = ((UART_BSCALE_VALUE << USART_BSCALE_gp) & USART_BSCALE_gm);
        UART_DEVICE.CTRLB = USART_RXEN_bm | USART_CLK2X_bm | USART_TXEN_bm;
        #endif // __AVR_XMEGA__

        #endif // USE_UART
        
        #ifdef USE_I2C
        // Initialize I2C interface
        #ifdef __AVR_XMEGA__
        I2C_DEVICE.CTRL = 0;
        #if I2C_MATCH_ANY
        I2C_DEVICE.SLAVE.CTRLA = TWI_SLAVE_ENABLE_bm | TWI_SLAVE_PMEN_bm;
        #else
        I2C_DEVICE.SLAVE.CTRLA = TWI_SLAVE_ENABLE_bm;
        #endif
        #if I2C_GC_ENABLE
        I2C_DEVICE.SLAVE.ADDR = I2C_ADDRESS | 1;
        #else
        I2C_DEVICE.SLAVE.ADDR = I2C_ADDRESS;
        #endif
        I2C_DEVICE.SLAVE.ADDRMASK = 0;
        #endif // __AVR_XMEGA__
        
        #ifdef USE_I2C_ADDRESS_NEGOTIATION
        I2C_AUTONEG_PORT.DIRCLR = (1 << I2C_AUTONEG_PIN);
        I2C_AUTONEG_PORT.OUTCLR = (1 << I2C_AUTONEG_PIN);
        #endif // USE_I2C_ADDRESS_NEGOTIATION
        
        #endif // USE_I2C
        
        // --------------------------------------------------
        // End initialization section
        
        // One time trigger section
        // Triggers that are checked once, regardless of
        // whether or not USE_ENTER_DELAY is selected
        // --------------------------------------------------
        
        
        
        // --------------------------------------------------
        // End one time trigger section
        
#ifdef USE_ENTER_DELAY
        k = ENTER_BLINK_COUNT*2;
        j = ENTER_BLINK_WAIT;
        while (!in_bootloader && k > 0)
        {
                if (j-- <= 0)
                {
                        #ifdef USE_LED
                        LED_PORT.OUTTGL = (1 << LED_PIN);
                        #endif // USE_LED
                        j = ENTER_BLINK_WAIT;
                        k--;
                }
#else // USE_ENTER_DELAY
                // Need a small delay when not running loop
                // so we don't accidentally enter the bootloader
                // on power-up with USE_ENTER_PIN selected
                asm("nop");
                asm("nop");
                asm("nop");
                asm("nop");
#endif // USE_ENTER_DELAY
                
                // Main trigger section
                // Set in_bootloader here to enter the bootloader
                // Checked when USE_ENTER_DELAY is selected
                // --------------------------------------------------
                
                #ifdef USE_ENTER_PIN
                // Check entry pin state
                if ((ENTER_PORT.IN & (1 << ENTER_PIN)) == (ENTER_PIN_STATE ? (1 << ENTER_PIN) : 0))
                        in_bootloader = 1;
                #endif // USE_ENTER_PIN
                
                #ifdef USE_ENTER_UART
                // Check for received character
                #ifdef __AVR_XMEGA__
                if (UART_DEVICE.STATUS & USART_RXCIF_bm)
                {
                        in_bootloader = 1;
                        comm_mode = MODE_UART;
                }
                #endif // __AVR_XMEGA__
                
                #endif // USE_ENTER_UART
                
                #ifdef USE_ENTER_I2C
                // Check for address match condition
                if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_APIF_bm) && 
                        (I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_AP_bm))
                {
                        in_bootloader = 1;
                        comm_mode = MODE_I2C;
                }
                #endif // USE_ENTER_I2C
                
                // --------------------------------------------------
                // End main trigger section
                
        #ifdef USE_ENTER_DELAY
        }
        #endif // USE_ENTER_DELAY
        
        // Main bootloader
        
        while (in_bootloader) {
                #ifdef USE_LED
                LED_PORT.OUTTGL = (1 << LED_PIN);
                #endif // USE_LED
                
                val = get_char();
                
                // Main bootloader parser
                // check autoincrement status
                if (val == 'a')
                {
                        // yes, it is supported
                        send_char('Y');
                }
                // Set address
                else if (val == 'A')
                {
                        // Read address high then low
                        address = (get_char() << 8) | get_char();
                        // acknowledge
                        send_char('\r');
                }
                // Extended address
                else if (val == 'H')
                {
                        // Read address high then low
                        address = ((unsigned long)get_char() << 16) | ((unsigned long)get_char() << 8) | get_char();
                        // acknowledge
                        send_char('\r');
                }
                // Chip erase
                else if (val == 'e')
                {
                        for (address = 0; address < APP_SECTION_SIZE; address += APP_SECTION_PAGE_SIZE)
                        {
                                // wait for SPM instruction to complete
                                SP_WaitForSPM();
                                // erase page
                                SP_EraseApplicationPage(address);
                        }
                        
                        // Randomize page buffer
                        EEPROM_LoadPage(&val);
                        // Erase EEPROM
                        EEPROM_EraseAll();
                        
                        // acknowledge
                        send_char('\r');
                }
                #ifdef ENABLE_BLOCK_SUPPORT
                // Check block load support
                else if (val == 'b')
                {
                        // yes, it is supported
                        send_char('Y');
                        // Send block size (page size)
                        send_char((APP_SECTION_PAGE_SIZE >> 8) & 0xFF);
                        send_char(APP_SECTION_PAGE_SIZE & 0xFF);
                }
                // Block load
                else if (val == 'B')
                {
                        // Block size
                        i = (get_char() << 8) | get_char();
                        // Memory type
                        val = get_char();
                        // Load it
                        send_char(BlockLoad(i, val, &address));
                }
                // Block read
                else if (val == 'g')
                {
                        // Block size
                        i = (get_char() << 8) | get_char();
                        // Memory type
                        val = get_char();
                        // Read it
                        BlockRead(i, val, &address);
                }
                #endif // ENABLE_BLOCK_SUPPORT
                #ifdef ENABLE_FLASH_BYTE_SUPPORT
                // Read program memory byte
                else if (val == 'R')
                {
                        SP_WaitForSPM();
                        send_char(SP_ReadByte((address << 1)+1));
                        send_char(SP_ReadByte((address << 1)+0));
                        
                        address++;
                }
                // Write program memory low byte
                else if (val == 'c')
                {
                        // get low byte
                        i = get_char();
                        send_char('\r');
                }
                // Write program memory high byte
                else if (val == 'C')
                {
                        // get high byte; combine
                        i |= (get_char() << 8);
                        SP_WaitForSPM();
                        SP_LoadFlashWord((address << 1), i);
                        address++;
                        send_char('\r');
                }
                // Write page
                else if (val == 'm')
                {
                        if (address >= (APP_SECTION_SIZE>>1))
                        {
                                // don't allow bootloader overwrite
                                send_char('?');
                        }
                        else
                        {
                                SP_WaitForSPM();
                                SP_WriteApplicationPage( address << 1);
                                send_char('\r');
                        }
                }
                #endif // ENABLE_FLASH_BYTE_SUPPORT
                #ifdef ENABLE_EEPROM_BYTE_SUPPORT
                // Write EEPROM memory
                else if (val == 'D')
                {
                        EEPROM_WriteByte( (unsigned char)(address / EEPROM_PAGE_SIZE), (unsigned char) (address & EEPROM_BYTE_ADDRESS_MASK), get_char() );
                        address++;
                }
                // Read EEPROM memory
                else if (val == 'd')
                {
                        send_char( EEPROM_ReadByte( (unsigned char)(address / EEPROM_PAGE_SIZE), (unsigned char) (address & EEPROM_BYTE_ADDRESS_MASK) ) );
                        address++;
                }
                #endif // ENABLE_EEPROM_BYTE_SUPPORT
                #ifdef ENABLE_LOCK_BITS
                // Write lockbits
                else if (val == 'l')
                {
                        SP_WaitForSPM();
                        SP_WriteLockBits( get_char() );
                        send_char('\r');
                }
                // Read lockbits
                else if (val == 'r')
                {
                        SP_WaitForSPM();
                        send_char(SP_ReadLockBits());
                }
                #endif // ENABLE_LOCK_BITS
                #ifdef ENABLE_FUSE_BITS
                // Read low fuse bits
                else if (val == 'F')
                {
                        SP_WaitForSPM();
                        send_char(SP_ReadFuseByte(0));
                }
                // Read high fuse bits
                else if (val == 'N')
                {
                        SP_WaitForSPM();
                        send_char(SP_ReadFuseByte(1));
                }
                // Read extended fuse bits
                else if (val == 'Q')
                {
                        SP_WaitForSPM();
                        send_char(SP_ReadFuseByte(2));
                }
                #endif // ENABLE_FUSE_BITS
                // Enter and leave programming mode
                else if ((val == 'P') || (val == 'L'))
                {
                        // just acknowledge
                        send_char('\r');
                }
                // Exit bootloader
                else if (val == 'E')
                {
                        in_bootloader = 0;
                        send_char('\r');
                }
                // Get programmer type
                else if (val == 'p')
                {
                        // serial
                        send_char('S');
                }
                // Return supported device codes
                else if (val == 't')
                {
                        // send only this device
                        send_char(123); // TODO
                        // terminator
                        send_char(0);
                }
                // Set LED, clear LED, and set device type
                else if ((val == 'x') || (val == 'y') || (val == 'T'))
                {
                        // discard parameter
                        get_char();
                        send_char('\r');
                }
                // Return program identifier
                else if (val == 'S')
                {
                        send_char('X');
                        send_char('B');
                        send_char('o');
                        send_char('o');
                        send_char('t');
                        send_char('+');
                        send_char('+');
                }
                // Read software version
                else if (val == 'V')
                {
                        send_char('1');
                        send_char('6');
                }
                // Read signature bytes
                else if (val == 's')
                {
                        send_char(SIGNATURE_2);
                        send_char(SIGNATURE_1);
                        send_char(SIGNATURE_0);
                }
                #ifdef USE_I2C
                #ifdef USE_I2C_ADDRESS_NEGOTIATION
                // Enter autonegotiate mode
                else if (val == '@')
                {
                        // The address autonegotiation protocol is borrowed from the
                        // OneWire address detection method.  The algorthim Utilizes
                        // one extra shared wire, pulled up by resistors just like the
                        // main I2C bus, a OneWire bus, or a wired-AND IRQ line.
                        // The protocol involves intelligently guessing all of the
                        // connected devices' 88 bit unique hardware ID numbers, stored
                        // permanently in the production signature row during manufacture
                        // (see XMega series datasheet for details)
                        #ifdef __AVR_XMEGA__
                        // k is temp
                        // devid is pointer to current bit, init to first bit
                        // of the hardware ID in the production signature row
                        devid_bit = 0x08 << 3;
                        // read first byte of hardware ID into temporary location
                        k = SP_ReadCalibrationByte(0x08);
                        
                        // main negotiation loop
                        while (1)
                        {
                                // wait for incoming data
                                while (1)
                                {
                                        // check for bit read command
                                        if (!(I2C_AUTONEG_PORT.IN & (1 << I2C_AUTONEG_PIN)))
                                        {
                                                // write current bit of hardware ID
                                                ow_slave_write_bit(k & 1);  // write bit
                                                break;
                                        }
                                        // check for I2C bus activity
                                        else if (I2C_DEVICE.SLAVE.STATUS & (TWI_SLAVE_APIF_bm | TWI_SLAVE_DIF_bm))
                                        {
                                                // grab a byte
                                                // (there will be no I2C bus activity while
                                                // the autonegotiation is taking place,
                                                // so it's OK to block)
                                                val = get_char();
                                                // Is this an address byte for me?
                                                if (val == '#')
                                                {
                                                        // If so, we're now attached, so light
                                                        // the LED and update the I2C bus
                                                        // controller accordingly
                                                        
                                                        // turn on attach LED
                                                        #ifdef USE_ATTACH_LED
                                                        #if ATTACH_LED_INV
                                                        ATTACH_LED_PORT.OUTCLR = (1 << ATTACH_LED_PIN);
                                                        #else
                                                        ATTACH_LED_PORT.OUTSET = (1 << ATTACH_LED_PIN);
                                                        #endif // ATTACH_LED_INV
                                                        #endif // USE_ATTACH_LED
                                                        
                                                        // get new address
                                                        #if I2C_AUTONEG_DIS_GC
                                                        I2C_DEVICE.SLAVE.ADDR = get_char() << 1;
                                                        #else
                                                        I2C_DEVICE.SLAVE.ADDR = (get_char() << 1) | 1;
                                                        #endif // I2C_AUTONEG_DIS_GC
                                                        
                                                        #if I2C_AUTONEG_DIS_PROMISC
                                                        // turn off promiscuous mode
                                                        I2C_DEVICE.SLAVE.CTRLA = TWI_SLAVE_ENABLE_bm;
                                                        #endif // I2C_AUTONEG_DIS_PROMISC
                                                        
                                                        // we're done here
                                                        goto autoneg_done;
                                                }
                                                // Check for sync command
                                                else if (val == 0x1b)
                                                {
                                                        // break out to main bootloader on sync
                                                        goto autoneg_done;
                                                }
                                        }
                                }
                                // Already wrote normal bit, so write the inverted one
                                ow_slave_write_bit(~k & 1); // write inverted bit
                                // Now read master's guess
                                i = ow_slave_read_bit();
                                // Does the guess agree with the current bit?
                                if ((k & 1 && i) || (~k & 1 && !i))
                                {
                                        // look at next bit
                                        devid_bit++;
                                        k >>= 1;
                                        
                                        // time for next byte?
                                        if (!(devid_bit & 7))
                                        {
                                                // Out of bits?
                                                if (devid_bit > (0x15 << 3))
                                                {
                                                        // Can't break here (need to wait
                                                        // to see if the master sends along
                                                        // an address) so wrap around instead
                                                        devid_bit = 0x08 << 3;
                                                }
                                                // there are some holes in the signature row,
                                                // so skip over them
                                                if (devid_bit == (0x0E << 3))
                                                        devid_bit += 0x02 << 3;
                                                if (devid_bit == (0x11 << 3))
                                                        devid_bit += 0x01 << 3;
                                                // Read next byte
                                                k = SP_ReadCalibrationByte(k >> 3);
                                        }
                                }
                                else
                                {
                                        // No match, we're done here
                                        break;
                                }
                        }
                        
autoneg_done:
                        // dummy to avoid error message
                        // this actually produces code 4 bytes smaller than either
                        // an asm nop, a continue, or a bare semicolon
                        i = 0;
                        
                        #endif
                }
                // out-of-order autonegotiate address message
                else if (val == '#')
                {
                        // ignore it
                        // (blocking to send a ? will cause trouble)
                }
                #endif // USE_I2C_ADDRESS_NEGOTIATION
                #endif // USE_I2C
                // ESC (0x1b) to sync
                // otherwise, error
                else if (val != 0x1b)
                {
                        send_char('?');
                }
        }
        
        // Wait for any lingering SPM instructions to finish
        SP_WaitForSPM();
        
        // Bootloader exit section
        // Code here runs after the bootloader has exited,
        // but before the application code has started
        // --------------------------------------------------
        
        #ifdef LOCK_SPM_ON_EXIT
        // Lock SPM writes
        SP_LockSPM();
        #endif // LOCK_SPM_ON_EXIT
        
        #ifdef USE_LED
        // Turn off LED on exit
        LED_PORT.DIRCLR = (1 << LED_PIN);
        #endif // USE_LED
        
        #ifdef USE_I2C_ADDRESS_NEGOTIATION
        #ifdef USE_ATTACH_LED
        // Disable ATTACH_LED
        ATTACH_LED_PORT.DIRCLR = (1 << ATTACH_LED_PIN);
        #endif // USE_ATTACH_LED
        #endif // USE_I2C_ADDRESS_NEGOTIATION
        
        // --------------------------------------------------
        // End bootloader exit section
        
        // Jump into main code
        EIND = 0x00;
        reset_vect();
}

#ifdef USE_I2C_ADDRESS_NEGOTIATION

#ifdef __AVR_XMEGA__
#define ow_assert()             I2C_AUTONEG_PORT.DIRSET = (1 << I2C_AUTONEG_PIN)
#define ow_deassert()           I2C_AUTONEG_PORT.DIRCLR = (1 << I2C_AUTONEG_PIN)
#define ow_read()               (I2C_AUTONEG_PORT.IN & (1 << I2C_AUTONEG_PIN))
#define ow_is_asserted()        (!ow_read())
#endif // __AVR_XMEGA__
#ifdef __AVR_MEGA__

#endif // __AVR_MEGA__

unsigned char __attribute__ ((noinline)) ow_slave_read_bit(void)
{
        unsigned char ret;
        ow_slave_wait_bit();
        _delay_us(12);
        ret = ow_read();
        _delay_us(8);
        return ret;
}

void __attribute__ ((noinline)) ow_slave_write_bit(unsigned char b)
{
        ow_slave_wait_bit();
        if (!b)
        {
                ow_assert();
        }
        _delay_us(20);
        ow_deassert();
}

void ow_slave_wait_bit(void)
{
        while (ow_read()) { };
}

#endif // USE_I2C_ADDRESS_NEGOTIATION

unsigned char __attribute__ ((noinline)) get_char(void)
{
        unsigned char ret;
        
        while (1)
        {
                #ifdef USE_UART
                // Get next character
                if (comm_mode == MODE_UNDEF || comm_mode == MODE_UART)
                {
                        #ifdef __AVR_XMEGA__
                        if (UART_DEVICE.STATUS & USART_RXCIF_bm)
                        {
                                comm_mode = MODE_UART;
                                return UART_DEVICE.DATA;
                        }
                        #endif // __AVR_XMEGA__
                }
                #endif // USE_UART
                
                #ifdef USE_I2C
                // Get next character
                if (comm_mode == MODE_UNDEF || comm_mode == MODE_I2C)
                {
                        #ifdef __AVR_XMEGA__
                        if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_APIF_bm) && 
                                (I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_AP_bm))
                        {
                                // Address match, send ACK
                                I2C_DEVICE.SLAVE.CTRLB = 0b00000011;
                                comm_mode = MODE_I2C;
                                first_byte = 1;
                        }
                        if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIF_bm) &&
                                !(I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIR_bm))
                        {
                                // Data has arrived
                                ret = I2C_DEVICE.SLAVE.DATA;
                                I2C_DEVICE.SLAVE.CTRLB = 0b00000011;
                                return ret;
                        }
                        if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIF_bm) &&
                                (I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIR_bm))
                        {
                                if (!first_byte && I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_RXACK_bm)
                                {
                                        I2C_DEVICE.SLAVE.CTRLB = 0b00000010; // end transaction
                                }
                                else
                                {
                                        first_byte = 0;
                                        // Wants data, but there is no data to send...
                                        // also include NAK
                                        I2C_DEVICE.SLAVE.DATA = '?';
                                        I2C_DEVICE.SLAVE.CTRLB = 0b00000110;
                                }
                        }
                        #endif // __AVR_XMEGA__
                }
                #endif // USE_I2C
        }
        
        return ret;
}

void __attribute__ ((noinline)) send_char(unsigned char c)
{
        unsigned char tmp;
        
        #ifdef USE_UART
        // Send character
        if (comm_mode == MODE_UNDEF || comm_mode == MODE_UART)
        {
                #ifdef __AVR_XMEGA__
                UART_DEVICE.DATA = c;
                while (!(UART_DEVICE.STATUS & USART_TXCIF_bm)) { }
                UART_DEVICE.STATUS |= USART_TXCIF_bm; // clear flag
                #endif // __AVR_XMEGA__
                
        }
        #endif // USE_UART
        
        #ifdef USE_I2C
        // Send character
        if (comm_mode == MODE_UNDEF || comm_mode == MODE_I2C)
        {
                while (1)
                {
                        #ifdef __AVR_XMEGA__
                        if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_APIF_bm) && 
                                (I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_AP_bm))
                        {
                                // Address match, send ACK
                                I2C_DEVICE.SLAVE.CTRLB = 0b00000011;
                                first_byte = 1;
                        }
                        if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIF_bm) &&
                                !(I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIR_bm))
                        {
                                // Data has arrived, ignore it
                                tmp = I2C_DEVICE.SLAVE.DATA;
                                I2C_DEVICE.SLAVE.CTRLB = 0b00000011;
                        }
                        if ((I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIF_bm) &&
                                (I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_DIR_bm))
                        {
                                if (!first_byte && I2C_DEVICE.SLAVE.STATUS & TWI_SLAVE_RXACK_bm)
                                {
                                        I2C_DEVICE.SLAVE.CTRLB = 0b00000010; // end transaction
                                }
                                else
                                {
                                        first_byte = 0;
                                        // Send data along
                                        I2C_DEVICE.SLAVE.DATA = c;
                                        I2C_DEVICE.SLAVE.CTRLB = 0b00000011;
                                }
                                return;
                        }
                        #endif // __AVR_XMEGA__
                }
        }
        #endif // USE_I2C
}

unsigned char BlockLoad(unsigned int size, unsigned char mem, ADDR_T *address)
{
        unsigned int data;
        ADDR_T tempaddress;
        
        // EEPROM memory type.
        if(mem == 'E')
        {
                unsigned char pageAddr, byteAddr, value;
                unsigned char buffer[APP_SECTION_PAGE_SIZE];
                
                EEPROM_FlushBuffer();
                // disable mapping of EEPROM into data space (enable IO mapped access)
                EEPROM_DisableMapping();
                
                // Fill buffer first, as EEPROM is too slow to copy with UART speed 
                for(tempaddress=0;tempaddress<size;tempaddress++){
                        buffer[tempaddress] = get_char();
                }
                
                // Then program the EEPROM
                for( tempaddress=0; tempaddress < size; tempaddress++)
                {
                        // void EEPROM_WriteByte( uint8_t pageAddr, uint8_t byteAddr, uint8_t value )
                        pageAddr = (unsigned char)( (*address) / EEPROM_PAGE_SIZE);
                        byteAddr = (unsigned char)( (*address) & EEPROM_BYTE_ADDRESS_MASK);
                        value = buffer[tempaddress];
                        
                        EEPROM_WriteByte(pageAddr, byteAddr, value);
                        
                        (*address)++; // Select next EEPROM byte
                }
                
                return '\r'; // Report programming OK
        } 
        
        // Flash memory type
        else if (mem == 'F' || mem == 'U')
        {
                // NOTE: For flash programming, 'address' is given in words.
                (*address) <<= 1; // Convert address to bytes temporarily.
                tempaddress = (*address);  // Store address in page.
                
                do
                {
                        data = get_char();
                        data |= (get_char() << 8);
                        SP_LoadFlashWord(*address, data);
                        (*address)+=2; // Select next word in memory.
                        size -= 2; // Reduce number of bytes to write by two.
                } while(size); // Loop until all bytes written.
                
                if (mem == 'F')
                {
                        SP_WriteApplicationPage(tempaddress);
                }
                else if (mem == 'U')
                {
                        SP_EraseUserSignatureRow();
                        SP_WaitForSPM();
                        SP_WriteUserSignatureRow();
                }
                
                SP_WaitForSPM();
                
                (*address) >>= 1; // Convert address back to Flash words again.
                return '\r'; // Report programming OK
        }

        // Invalid memory type?
        else
        {
                return '?';
        }
}



void BlockRead(unsigned int size, unsigned char mem, ADDR_T *address)
{
        // EEPROM memory type.
        
        if (mem == 'E') // Read EEPROM
        {
                unsigned char byteAddr, pageAddr;
                
                EEPROM_DisableMapping();
                EEPROM_FlushBuffer();
                
                do
                {
                        pageAddr = (unsigned char)(*address / EEPROM_PAGE_SIZE);
                        byteAddr = (unsigned char)(*address & EEPROM_BYTE_ADDRESS_MASK);
                        
                        send_char( EEPROM_ReadByte( pageAddr, byteAddr ) );
                        // Select next EEPROM byte
                        (*address)++;
                        size--; // Decrease number of bytes to read
                } while (size); // Repeat until all block has been read
        }
        
        // Flash memory type.
        else if (mem == 'F' || mem == 'U' || mem == 'P')
        {
                (*address) <<= 1; // Convert address to bytes temporarily.
                
                do
                {
                        if (mem == 'F')
                        {
                                send_char( SP_ReadByte( *address) );
                                send_char( SP_ReadByte( (*address)+1) );
                        }
                        else if (mem == 'U')
                        {
                                send_char( SP_ReadUserSignatureByte( *address) );
                                send_char( SP_ReadUserSignatureByte( (*address)+1) );
                        }
                        else if (mem == 'P')
                        {
                                send_char( SP_ReadCalibrationByte( *address) );
                                send_char( SP_ReadCalibrationByte( (*address)+1) );
                        }
                        
                        SP_WaitForSPM();
                        
                        (*address) += 2;    // Select next word in memory.
                        size -= 2;          // Subtract two bytes from number of bytes to read
                } while (size);         // Repeat until all block has been read
                
                (*address) >>= 1;       // Convert address back to Flash words again.
        }
}

