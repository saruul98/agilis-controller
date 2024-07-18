#include "agilis_controller.h"

/**
 * @brief Reads from commfd and return a string without spaces
 */
std::string read_cpp(int commfd) {
    unsigned char *c;
    c = (unsigned char *)malloc(100);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    int i = read(commfd, c, 100);
    std::string str (reinterpret_cast<char const*>(c));

    //wprintrw(win,reinterpret_cast< char const* >(c));
    //wprintrw(win,"\n");

    if(i > 2)
        str.resize(i-2);
    else
        if(i != 0)
            printf("Warning: Cant read return string\n" );

    //str.erase(std::remove_if(str.begin(), str.end(), isspace), str.end());
    return str;
}


/**
 * @brief write Ascii command buf to commfd
 */
void write_cpp(std::string buf, int commfd) {
    write(commfd, buf.c_str(), buf.size());
    tcdrain(commfd);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}


/**
 * @brief write Ascii command buf to commfd and if check_for_error is true the returned error code is checked
 */
void write_comm(std::string buf, int commfd, bool check_for_error) {
    write_cpp(buf, commfd);
    if(check_for_error) {
        write_cpp("TE\r\n", commfd);
        std::string res = read_cpp(commfd);
        if(res != "TE0") {
            printf("Agilis controller returned error code %s\n", res);
        }
    }
}


/**
 * @brief read_limit Tell limit status
 * @param commfd  file descriptor
 * @param axis Number of axis (1 or 2) which is checked
 * @return true if limit switch is active
 */
bool read_limit(int commfd, int axis) {
    std::string buf;
    buf="PH\r\n";
    write_comm(buf, commfd, false);
    bool limit = false;
    std::string res = read_cpp(commfd);

    if (res == "PH3") {
        std::cout << "Limit switch of channel #1 and channel #2 are active" << std::endl;
        limit = true;
        return limit;
    }

    if (res == "PH2") {
        std::cout << "Limit switch of channel #2 is active, limit switch of channel #1 is not active " << std::endl;
        if(axis == 2)
            limit = true;
        return limit;
    }

    if (res == "PH1") {
        std::cout << "Limit switch of channel #1 is active, limit switch of channel #2 is not active " << std::endl;
        if(axis == 1)
            limit = true;
        return limit;
    }

    if (res == "PH0") {
        // std::cout << "No limit switch is active" << std::endl;
        return limit;
    }

    return limit;
}


/**
 * @brief init_actuators Configure com port for AG-UC8
 * @return
 */
int init_actuators(std::string tty) {

    printf("Actuator initialisation...\n");

    int controller = open(tty.c_str(),  O_RDWR | O_NONBLOCK );
    //  int controller = open("/sys/bus/usb-serial/devices/ttyUSB0",  O_RDWR | O_NONBLOCK );

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    if (controller == -1 ) {
        std::cerr << "Unable to open " << tty << std::endl;
        std::cout << "Exiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        exit(42);
    }

    std::cout << "Opened " << tty << std::endl;

    struct termios   options;    // Terminal options
    int              rc;         // Return value
    int pgrp = tcgetpgrp (controller);

    if (pgrp >= 0) {
        std::cerr << "Another program is using the serial device!"<< std::endl;
        std::cout << "Exiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        exit(42);
    }

    std::cout << "Acquired access to the serial device" << std::endl;

    tcsetpgrp (controller, getpgrp ());

    // Get the current options for the port
    if((rc = tcgetattr(controller, &options)) < 0) {
        std::cerr << "Failed to get attributes from the actuator controller!" << std::endl;
        std::cout << "Exiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        exit(42);
    }

    std::cout << "Acquired attributes from the actuator controller" << std::endl;

    std::cout << "Configuring device..." << std::endl;

    // Raw I/O
    cfmakeraw (&options);

    // Set the baud rates
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // 8 Bits
    options.c_cflag     &=  ~CSIZE;
    options.c_cflag     |=  CS8;

    // No parity
    options.c_cflag     &=  ~PARENB;

    // 1 Stop Bit
    options.c_cflag     &=  ~CSTOPB;

    // No flow control
    options.c_cflag     &=  ~CRTSCTS;
    options.c_iflag     &=  ~IXON;
    options.c_iflag     &=  ~IXOFF;

    // Non-blocking READ
    options.c_cc[VMIN]  =   1;
    options.c_cc[VTIME] =   0;

    // turn on READ & ignore ctrl lines
    options.c_cflag     |=  CREAD | CLOCAL;

    /* Flush Port, then applies attributes */
    tcflush(controller, TCIOFLUSH);

    if(tcsetattr(controller, TCSANOW, &options) != 0) {
        std::cout << "Error " << errno << " from tcsetattr" << std::endl;
        std::cout << "Exiting..." << std::endl;
        exit(42);
    }
    std::cout << "Sending RESET command..." << std::endl;
    write_comm("RS\r\n", controller, true);
    std::cout << "Sending REMOTE MODE command..." << std::endl;
    write_comm("MR\r\n", controller, true);

    std::cout << "Actuator initialization complete" << std::endl;
    std::cout << "Success!" << std::endl;

    return controller;
}

/**
 * @brief Applies steps to axis on channel
 */
void step_to_axis_channel(int steps, int axis, int channel, int controller) {
    //Set Channel Number
    std::stringstream cc;
    cc << "CC" << channel << "\r\n";
    write_comm(cc.str().c_str(),controller, true);

    //Initiate relative Movement to Axis
    std::stringstream st;
    st << axis << "PR" << steps << "\r\n";
    write_comm(st.str().c_str(),controller, true);

    //Check if one Axis end up in Limit Switch
    read_limit(controller, axis);
}

/**
 * @brief Applies  movement to axis on channel
Starts a process to move to an absolute
position. During the execution of the
command, the USB communication is interrupted. After completion, the
communication is opened again. The execution of the command can last up to 2
minutes.

pos target position in 1/1000th of the total travel.
 */
void move_to_axis_channel(int pos, int axis, int channel, int controller) {
    //Set Channel Number
    std::stringstream cc;
    cc << "CC" << channel << "\r\n";
    write_comm(cc.str().c_str(),controller, true);

    //Initiate relative Movement to Axis
    std::stringstream st;
    st << axis << "PA" << pos << "\r\n";
    write_comm(st.str().c_str(),controller, true);
    std::this_thread::sleep_for(std::chrono::seconds(60));

    read_limit(controller, axis);
    //Check if one Axis end up in Limit Switch
    // bool is_limit=read_limit(controller,axis);
}


void get_axis_channel_status(int axis, int channel, int controller) {
    //Set Channel Number
    std::stringstream cc;
    cc << "CC" << channel << "\r\n";
    write_comm(cc.str().c_str(), controller, true);

    // Check status of axis
    std::stringstream st;
    st << axis << "TS" << "\r\n";
    write_comm(st.str().c_str(), controller, false);

    std::string res = read_cpp(controller);

    if (res == "TE0")
        std::cout << "Channel " << channel << " axis " << axis << "is Ready" << std::endl;
    else if(res == "TE1")
        std::cout << "Channel " << channel << " axis " << axis << "is Stepping" << std::endl;
    else if(res == "TE2")
        std::cout << "Channel " << channel << " axis " << axis << "is Jogging" << std::endl;
    else if(res == "TE3")
        std::cout << "Channel " << channel << " axis " << axis << "is Moving to limit" << std::endl;

}

