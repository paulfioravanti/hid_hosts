"use strict";

const HID = require("node-hid");
const COMMAND = parseInt(process.argv[2]); // first command line argument

// Keyboard info
const KEYBOARD_VENDOR_ID =  0xFEED;
const KEYBOARD_PRODUCT_ID = 0x1337;

const device = new HID.HID(KEYBOARD_VENDOR_ID, KEYBOARD_PRODUCT_ID);
device.write([COMMAND]);
device.close();
