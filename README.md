# rb_robout_common

Common packages of the RB-Robout: URDF description of the RB-Robout, platform messages and other files for simulation.

## Packages

### rb_robout_control

This package contains the launch and configuration files to spawn the joint controllers with the ROS controller_manager. It allows to launch the joint controllers for the:
- RB-Robout (4 omidirectional wheels)

The RB-Robout simulation stack follows the gazebo_ros controller manager scheme described in
[http://gazebosim.org/tutorials](http://gazebosim.org/tutorials)

### rb_robout_description

The urdf, meshes, and other elements needed in the description are contained here. This package includes the description of the RB-Robout mobile platform.
The package includes also some launch files to publish the robot state and to test the urdf files in rviz.

### rb_robout_localization

This package contains launch files related to mapping and localization with the RB-Robout robot.

### rb_robout_navigation

This package contains all the configuration and launch files needed to execute navigation tasks.

### rb_robout_pad

This package contains the node that subscribes to /joy messages and publishes command messages for the robot platform including speed level control. The joystick output is feed to a mux [http://wiki.ros.org/twist_mux](http://wiki.ros.org/twist_mux) so that the final command to the robot can be set by different components (move_base, etc.)

The node allows to load different types of joysticks (PS4, PS3, Logitech, Thrustmaster). New models can be easily added by creating new .yaml files. If modbus_io node is available, the digital outputs (ligths, axes, etc.) can also be controlled with the pad. If ptz camera is available, the pan-tilt-zoom can also be commanded with the pad.
