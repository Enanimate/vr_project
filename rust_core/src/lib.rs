use std::{ffi::{CStr, c_char}, io::{BufRead, BufReader}, time::Duration};

use serde::Deserialize;

struct VRLayout {
    hp_led_one: Vec3,
    hp_led_two: Vec3,
    hp_led_three: Vec3,
}

impl VRLayout {
    pub fn new() -> Self {
        Self {
            hp_led_one: Vec3 {
                x: 0.0,
                y: 30.0,
                z: 40.0
            },
            hp_led_two: Vec3 {
                x: -50.0,
                y: 0.0,
                z: 10.0
            },
            hp_led_three: Vec3 {
                x: 50.0,
                y: 0.0,
                z: 10.0
            }
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Quaternion {
    pub w: f64,
    pub x: f64,
    pub y: f64,
    pub z: f64
}

#[repr(C)]
pub struct Vec3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

#[derive(Deserialize)]
struct QuaternionJson {
    w: f64,
    x: f64,
    y: f64,
    z: f64,
}

pub struct VRDevice {
    headset_layout: VRLayout,
    serial_port: Option<Box<dyn serialport::SerialPort>>,
    current_pose: Quaternion,
    connected: bool,
}

impl VRDevice {
    fn new() -> Self {
        VRDevice {
            headset_layout: VRLayout::new(),
            serial_port: None,
            current_pose: Quaternion { w: 1.0, x: 0.0, y: 0.0, z: 0.0 },
            connected: false
        }
    }

    fn connect(&mut self, port: &str) -> bool {
        match serialport::new(port, 115200)
            .timeout(Duration::from_millis(100))
            .open() 
        {
            Ok(serial_port) => {
                self.serial_port = Some(serial_port);
                self.connected = true;
                println!("Connected to {}", port);
                true
            }

            Err(e) => {
                eprintln!("Failed to open {}: {}", port, e);
                self.connected = false;
                false
            }
        }
    }

    fn read_quaternion(&mut self) -> Option<Quaternion> {
        let port = self.serial_port.as_mut()?;

        let mut reader = BufReader::new(port);
        let mut line = String::new();

        match reader.read_line(&mut line) {
            Ok(0) => {
                return None;
            }

            Ok(_) => {
                match serde_json::from_str::<QuaternionJson>(&line.trim()) {
                    Ok(quat) => {
                        self.current_pose = Quaternion {
                            w: quat.w,
                            x: quat.x,
                            y: quat.y,
                            z: quat.z,
                        };
                        Some(self.current_pose)
                    }

                    Err(e) => {
                        eprintln!("JSON parse error: {} (line: {})", e, line.trim());
                        None
                    }
                }
            }

            Err(e) => {
                // Don't disconnect on timeout - it's normal if Arduino sends data slowly
                // Only disconnect on actual I/O errors
                if e.kind() != std::io::ErrorKind::TimedOut {
                    eprintln!("Serial read error: {}", e);
                    self.connected = false;
                }
                None
            }
        }
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_create(port_name: *const c_char) -> *mut VRDevice {
    let port = unsafe {
        if port_name.is_null() {
            return std::ptr::null_mut();
        }
        match CStr::from_ptr(port_name).to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    let mut device = Box::new(VRDevice::new());

    if device.connect(port) {
        Box::into_raw(device)
    } else {
        std::ptr::null_mut()
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_update(device: *mut VRDevice) -> u8 {
    if device.is_null() {
        return 0;
    }

    let device = unsafe { &mut *device };

    match device.read_quaternion() {
        Some(_) => 1,
        None => 0,
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_get_pose(device: *const VRDevice, out_quat: *mut Quaternion) {
    if device.is_null() || out_quat.is_null() {
        return;
    }

    let device = unsafe { &*device };
    let out = unsafe { &mut *out_quat };

    *out = device.current_pose;
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_get_position(device: *const VRDevice, out_pos: *mut Vec3) {
    
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_is_connected(device: *const VRDevice) -> u8 {
    if device.is_null() {
        return 0;
    }

    let device = unsafe { &*device };
    if device.connected { 1 } else { 0 }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_destroy(device: *mut VRDevice) {
    if !device.is_null() {
        unsafe {
            let _ = Box::from_raw(device);
        }
    }
}