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
     * Gets the 8bit memory address associated with a given register.
     * This should be used rather than accessing the 8bit registers directly
     * through the tables, as (HL) is a special case which this takes care of.
     *
     * @param reg_no The register number to get in the r table
     * @return A pointer to the register
     */
    inline uint8_t *get_r_reg(uint8_t reg_no)
    {
        if(reg_no == 6)
        {
            return &memory[reg.general.HL];
        }

        return reg_table_r[reg_no];
    }

    /*!
     * Gets the 16bit memory address associated with a given register.
     * This should be used rather than accessing the 16bit registers directly
     * through the tables.
     *
     * @param reg_no The register number to get in the rp table
     * @return A pointer to the register
     */
    inline uint16_t *get_rp_reg(uint8_t reg_no)
    {
        return reg_table_rp[reg_no];
    }

    /*!
     * Gets the 16bit memory address associated with a given register.
     * This should be used rather than accessing the 16bit registers directly
     * through the tables.
     *
     * @param reg_no The register number to get in the rp2 table
     * @return A pointer to the register
     */
    inline uint16_t *get_rp2_reg(uint8_t reg_no)
    {
        return reg_table_rp2[reg_no];
    }

    /*!
     * Gets the value of a given condition flag value
     *
     * @param cc_no The condition value to get
     * @return The value belonging to that condition
     */
    inline bool get_cc_value(uint8_t cc_no)
    {
        switch(cc_no)
        {
            case 0:
                return !reg.general.F.Z;
            case 1:
                return reg.general.F.Z;
            case 2:
                return !reg.general.F.C;
            case 3:
                return reg.general.F.C;
            case 4:
                return !reg.general.F.PV;
            case 5:
                return reg.general.F.PV;
            case 6:
                return !reg.general.F.S;
            case 7:
                return reg.general.F.S;
            default:
                abort();
        }
    }

    /*!
     * Checks the parity of two numbers
     *
     * @tparam T The type of thing to calculate the parity of
     * @param num The thing to calculate the parity of
     * @return True if the parity is even, false otherwise
     */
    template<typename T>
    inline bool parity(T num)
    {
        static_assert(((sizeof(T) * 8) % 2) == 0, "Can't check parity of an object which is not even in size");
        size_t a = sizeof(T) * 8;
        do
        {
            a /= 2;
            num ^= num >> 16;
        } while(a != 1);
        return (~num) & 1;
    }

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
    std::array<std::string, 8> cc_table_names;
};


#endif //Z80_DISASSEMBLER_DECODER_H
