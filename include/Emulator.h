//
// Created by fred on 11/05/18.
//

#ifndef Z80_DISASSEMBLER_DECODER_H
#define Z80_DISASSEMBLER_DECODER_H


#include <string>
#include <memory>
#include <vector>
#include <functional>

class Emulator
{
public:
    enum PortState
    {
        Read = 0,
        Write = 1,
    };
    typedef std::function<void(PortState, uint8_t *data, uint16_t)> port_handler_t;

    enum Prefix
    {
        None = 0,
        CB = 1,
        DD = 2,
        ED = 3,
        FD = 4,
        PrefixCount = 5
    };

    /*!
     * Constructor
     */
    Emulator();

    /*!
     * Resets the state of the emulator
     */
    void reset();

    /*!
     * Emulates instruction data
     *
     * @param data The data to emulate
     * @param log_stream The data stream to log to
     */
    void emulate(const std::vector<uint8_t> &data, std::ostream &log_stream);

    /*!
     * Registers a port handler, this functor
     * will be called whenever the CPU tries to read
     * or write to a port.
     *
     * @param port_no The port number to install the handler into
     * @param handler The functor to call on read/write request
     */
    void bind_port(uint16_t port_no, port_handler_t handler);
private:

    /*!
     * Converts a value into its associated prefix
     *
     * @param val The value to convert
     * @return The prefix, or None if none.
     */
    Prefix to_prefix(uint8_t val);

    // CPU Functions

    /*!
     * Pushes a 16bit value to the stack in memory and decrements SP by 2
     *
     * @param val The value to push
     */
    void push(uint16_t val);

    /*!
     * Pops a 16bit value from the top of the stack in memory and increments SP by 2
     *
     * @return The popped value
     */
    uint16_t pop();

    /*!
     * Writes data to a port
     *
     * @param port The port number to write to
     * @param n A pointer to the data to write
     */
    inline void out(uint16_t port, uint8_t *n)
    {
        ports[port](PortState::Write, n, 1);
    }

    /*!
     * Reads data from a port
     *
     * @param port The port number to read from
     * @param n A pointer to read the data into
     */
    inline void in(uint16_t port, uint8_t *n)
    {
        ports[port](PortState::Read, n, 1);
    }

    // CPU State

    // Memory
    unsigned char memory[0x10000];

    // Ports
    port_handler_t ports[0x10000];

    // Registers
    struct Registers
    {
        struct
        {
            union
            {
                struct
                {
                    uint8_t B;
                    uint8_t C;
                };
                uint16_t BC;
            };

            union
            {
                struct
                {
                    uint8_t D;
                    uint8_t E;
                };
                uint16_t DE;
            };

            union
            {
                struct
                {
                    uint8_t H;
                    uint8_t L;
                };
                uint16_t HL;
            };

            union
            {
                struct
                {
                    uint8_t A;
                    struct
                    {
                        uint8_t C:1;  // Carry
                        uint8_t N:1;  // Subtract
                        uint8_t PV:1; // Parity/Overflow
                        uint8_t H:1;  // Half Carry
                        uint8_t Z:1;  // Zero
                        uint8_t S:1;  // Sign
                    } F;
                };
                uint16_t AF;
            } ;
        } general, shadow;

        uint16_t SP;
        uint16_t PC;
    } reg;

    // Register lookup tables
    std::array<uint8_t*, 8> reg_table_r;
    std::array<uint16_t*, 4> reg_table_rp;
    std::array<uint16_t*, 4> reg_table_rp2;

    std::array<std::string, 8> reg_table_r_names;
    std::array<std::string, 4> reg_table_rp_names;
    std::array<std::string, 4> reg_table_rp2_names;
};


#endif //Z80_DISASSEMBLER_DECODER_H
