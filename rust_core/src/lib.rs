use std::{ffi::{CStr, c_char}, fs::OpenOptions, io::{BufRead, BufReader, Write}, sync::{Arc, Mutex}, thread, time::Duration};

use serde::Deserialize;

#[repr(C)]
#[derive(Clone, Copy)]
pub struct IRBlob {
    pub x: u16,
    pub y: u16,
    pub size: u8,
}

#[derive(Deserialize)]
struct IRData {
    ir: Vec<IRBlobJson>,
}

#[derive(Deserialize)]
struct IRBlobJson {
    x: u16,
    y: u16,
    s: u8,
}

struct TrackingData {
    quaternion: Quaternion,
    ir_blobs: Vec<IRBlob>,
    smoothed_position: Vec3,
    connected: bool,
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
#[derive(Clone, Copy)]
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
    tracking_data: Arc<Mutex<TrackingData>>,
    headset_thread: Option<thread::JoinHandle<()>>,
    tracking_thread: Option<thread::JoinHandle<()>>,
}

impl VRDevice {
    fn new() -> Self {
        VRDevice {
            tracking_data: Arc::new(Mutex::new(TrackingData {
                quaternion: Quaternion { w: 1.0, x: 0.0, y: 0.0, z: 0.0 },
                ir_blobs: Vec::new(),
                smoothed_position: Vec3 { x: 0.0, y: 0.0, z: 0.0 },
                connected: false,
            })),
            headset_thread: None,
            tracking_thread: None,
        }
    }

    fn connect(&mut self, headset_port: &str, tracking_port: &str) -> bool {
        // Open headset serial port (COM4)
        let headset_serial = match serialport::new(headset_port, 115200)
            .timeout(Duration::from_millis(100))
            .open() 
        {
            Ok(port) => {
                println!("Connected to headset port: {headset_port}");
                port
            }

            Err(e) => {
                eprintln!("Failed to open {}: {}", headset_port, e);
                return false;
            }
        };

        // Open tracking serial port (COM3)
        let tracking_serial = match serialport::new(tracking_port, 115200)
            .timeout(Duration::from_millis(100))
            .open() 
        {
            Ok(port) => {
                println!("Connected to tracking port: {tracking_port}");
                port
            }

            Err(e) => {
                eprintln!("Failed to open {}: {}", tracking_port, e);
                return false;
            }
        };

        let tracking_data_headset = Arc::clone(&self.tracking_data);
        let tracking_data_tracking = Arc::clone(&self.tracking_data);

        // Set initially connected
        {
            let mut data = self.tracking_data.lock().unwrap();
            data.connected = true;
        }

        // Spawn headset thread (reads quaternion from COM4)
        let headset_thread = thread::spawn(move || {
            let mut reader = BufReader::new(headset_serial);
            let mut line = String::new();

            loop {
                line.clear();
                match reader.read_line(&mut line) {
                    Ok(0) => break,
                    Ok(_) => {
                        if let Ok(quat) = serde_json::from_str::<QuaternionJson>(line.trim()) {
                            let mut data = tracking_data_headset.lock().unwrap();
                            data.quaternion = Quaternion {
                                w: quat.w,
                                x: quat.x,
                                y: quat.y,
                                z: quat.z,
                            };
                        }
                    }
                    Err(e) => {

                        if e.kind() != std::io::ErrorKind::TimedOut {
                            eprintln!("Headset serial error: {}", e);
                            let mut data = tracking_data_headset.lock().unwrap();
                            data.connected = false;
                            break;
                        }
                    }
                }
            }
        });

        // Spawn tracking thread (reads IR blobs from COM3)
        let tracking_thread = thread::spawn(move || {
            let mut reader = BufReader::new(tracking_serial);
            let mut line = String::new();

            loop {
                line.clear();
                match reader.read_line(&mut line) {
                    Ok(0) => break,
                    Ok(_) => {
                        if let Ok(ir_data) = serde_json::from_str::<IRData>(line.trim()) {
                            let mut data = tracking_data_tracking.lock().unwrap();
                            data.ir_blobs = ir_data.ir.iter().map(|blob| IRBlob {
                                x: blob.x,
                                y: blob.y,
                                size: blob.s,
                            }).collect();
                        }
                    }
                    Err(e) => {
                        if e.kind() != std::io::ErrorKind::TimedOut {
                            eprintln!("Tracking serial error: {e}");
                            let mut data = tracking_data_tracking.lock().unwrap();
                            data.connected = false;
                            break;
                        }
                    }
                }
            }
        });

        self.headset_thread = Some(headset_thread);
        self.tracking_thread = Some(tracking_thread);

        true
    }

    // Calculate distance between two 2D points
    fn calculate_pixel_distance(blob1: &IRBlob, blob2: &IRBlob) -> f64 {
        let dx = (blob1.x as f64) - (blob2.x as f64);
        let dy = (blob1.y as f64) - (blob2.y as f64);
        (dx * dx + dy * dy).sqrt()
    }

    // Estimate Z-depth using perspective projection
    fn estimate_depth(&self, ir_blobs: &[IRBlob]) -> Option<f64> {
        if ir_blobs.len() < 2 {
            return None;
        }

        // Calculate depth from ALL pairs of blobs and average
        let mut depth_sum = 0.0;
        let mut count = 0;

        for i in 0..ir_blobs.len() {
            for j in (i+1)..ir_blobs.len() {
                let pixel_distance = Self::calculate_pixel_distance(&ir_blobs[i], &ir_blobs[j]);

                if pixel_distance < 1.0 {
                    continue;
                }

                // Average physical distance between LEDs (approximate)
                // Real distances: 69mm, 97mm, 111mm, etc - average ~85mm
                const AVG_LED_SPACING: f64 = 0.085;

                const FOCAL_LENGTH: f64 = 1728.0;

                // Perspective formula
                let depth = (AVG_LED_SPACING * FOCAL_LENGTH) / pixel_distance;
                depth_sum += depth;
                count += 1;
            }
        }

        if count > 0 {
            Some(depth_sum / (count as f64))
        } else {
            None
        }
    }

    // Estimate full 3D position from IR blobs
    fn estimate_position(&self, ir_blobs: &[IRBlob]) -> Option<Vec3> {
        if ir_blobs.is_empty() {
            return None;
        }

        // Calculate Z-depth first
        let depth = self.estimate_depth(ir_blobs)?;

        // Calculate centroid
        let mut sum_x = 0.0;
        let mut sum_y = 0.0;

        for blob in ir_blobs {
            sum_x += blob.x as f64;
            sum_y += blob.y as f64;
        }

        let centroid_x = sum_x / (ir_blobs.len() as f64);
        let centroid_y = sum_y / (ir_blobs.len() as f64);

        // Wiimote IR camera specs (33° horizontal FOV, 1024×768 output)
        const CAMERA_CENTER_X: f64 = 512.0;
        const CAMERA_CENTER_Y: f64 = 384.0;
        const FOCAL_LENGTH: f64 = 1728.0;  // (1024/2) / tan(33°/2)

        let offset_x = centroid_x - CAMERA_CENTER_X;
        let offset_y = centroid_y - CAMERA_CENTER_Y;

        // Convert pixel offset to real-world position
        // Note: IR Sensor Y is inverted
        let x = (offset_x * depth) / FOCAL_LENGTH;
        let y = -(offset_y * depth) / FOCAL_LENGTH;
        let z = -depth;

        // Write debug output to file
        if let Ok(mut file) = OpenOptions::new()
            .create(true)
            .append(true)
            .open("C:\\Users\\spa07\\Documents\\Dev\\vr_driver\\tracking_debug.log")
        {
            let _ = writeln!(file, "Position: X={:.3}, Y={:.3}, Z={:.3} (depth={:.3}m, blobs={})",
                           x, y, z, depth, ir_blobs.len());
        }

        Some(Vec3 { x, y, z})
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_create(headset_port_name: *const c_char, tracking_port_name: *const c_char) -> *mut VRDevice {
    let headset_port = unsafe {
        if headset_port_name.is_null() {
            return std::ptr::null_mut();
        }
        match CStr::from_ptr(headset_port_name).to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    let tracking_port = unsafe {
        if tracking_port_name.is_null() {
            return std::ptr::null_mut();
        }
        match CStr::from_ptr(tracking_port_name).to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    let mut device = Box::new(VRDevice::new());

    if device.connect(headset_port, tracking_port) {
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

    let device = unsafe { &*device };

    let data = device.tracking_data.lock().unwrap();
    if data.connected { 1 } else { 0 }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_get_pose(device: *const VRDevice, out_quat: *mut Quaternion) {
    if device.is_null() || out_quat.is_null() {
        return;
    }

    let device = unsafe { &*device };
    let out = unsafe { &mut *out_quat };

    let data = device.tracking_data.lock().unwrap();
    *out = data.quaternion;
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_get_position(device: *const VRDevice, out_pos: *mut Vec3) {
    if device.is_null() || out_pos.is_null() {
        return;
    }

    let device = unsafe { &*device };
    let out = unsafe { &mut *out_pos };

    let mut data = device.tracking_data.lock().unwrap();

    // Estimate raw position from IR blobs
    if let Some(raw_position) = device.estimate_position(&data.ir_blobs) {
        // Reject outliers (depth suddenly changed by more than 50cm)
        let depth_change = (raw_position.z - data.smoothed_position.z).abs();
        if depth_change > 0.5 && data.smoothed_position.z != 0.0 {
            // Outlier detected - skip this frame, use old smoothed position
            *out = data.smoothed_position;
            return;
        }
        // Apply exponention moving average filter (low-pass filter)
        const SMOOTHING: f64 = 0.9;

        let smoothed = Vec3 {
            x: data.smoothed_position.x * SMOOTHING + raw_position.x * (1.0 - SMOOTHING),
            y: data.smoothed_position.y * SMOOTHING + raw_position.y * (1.0 - SMOOTHING),
            z: data.smoothed_position.z * SMOOTHING + raw_position.z * (1.0 - SMOOTHING),
        };

        // Store for next frame
        data.smoothed_position = smoothed;

        // Amplify movement (optional)
        const POSITION_SCALE: f64 = 2.0;

        *out = Vec3 {
            x: smoothed.x * POSITION_SCALE,
            y: smoothed.y * POSITION_SCALE,
            z: smoothed.z * POSITION_SCALE,
        };
    } else {
        // No position available - return last smoothed position
        *out = data.smoothed_position;
    }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_is_connected(device: *const VRDevice) -> u8 {
    if device.is_null() {
        return 0;
    }

    let device = unsafe { &*device };
    
    let data = device.tracking_data.lock().unwrap();
    if data.connected { 1 } else { 0 }
}

#[unsafe(no_mangle)]
pub extern "C" fn vr_device_destroy(device: *mut VRDevice) {
    if !device.is_null() {
        unsafe {
            let _ = Box::from_raw(device);
        }
    }
}