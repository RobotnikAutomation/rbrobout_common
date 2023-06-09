/*
 * rbrobout_pad
 * Copyright (c) 2012, Robotnik Automation, SLL
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Robotnik Automation, SLL. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \author Robotnik Automation, SLL
 * \brief Allows to use a pad with the roboy controller, sending the messages received from the joystick device
 */


#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <geometry_msgs/Twist.h>
#include <robotnik_msgs/ptz.h>
// Not yet catkinized 9/2013
// #include <sound_play/sound_play.h>
#include <unistd.h>
#include <robotnik_msgs/set_mode.h>
#include <robotnik_msgs/set_digital_output.h>
#include <std_srvs/SetBool.h>
#include <robotnik_msgs/ptz.h>
#include <robotnik_msgs/home.h>
#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/publisher.h>

#define DEFAULT_NUM_OF_BUTTONS		16
#define DEFAULT_AXIS_LINEAR_X		1
#define DEFAULT_AXIS_LINEAR_Y       0
#define DEFAULT_AXIS_ANGULAR		2
#define DEFAULT_AXIS_LINEAR_Z       3
#define DEFAULT_SCALE_LINEAR		1.0
#define DEFAULT_SCALE_ANGULAR		2.0
#define DEFAULT_SCALE_LINEAR_Z      1.0

#define ITERATIONS_WRITE_MODBUS		2

//Used only with ps4
#define AXIS_PTZ_TILT_UP  0
#define AXIS_PTZ_TILT_DOWN  1
#define AXIS_PTZ_PAN_LEFT  2
#define AXIS_PTZ_PAN_RIGHT  3


////////////////////////////////////////////////////////////////////////
//                               NOTE:                                //
// This configuration is made for a THRUSTMASTER T-Wireless 3in1 Joy  //
//   please feel free to modify to adapt for your own joystick.       //
// 								      //


class RBRoboutPad
{
	public:
	RBRoboutPad();
	void Update();

	private:
	void padCallback(const sensor_msgs::Joy::ConstPtr& joy);
	int setBumperOverride(bool value);
	int setManualRelease(bool value);
	ros::NodeHandle nh_;
	ros::NodeHandle pnh_;

	int manual_release_true_number_, manual_release_false_number_, bumper_override_false_number_, bumper_override_true_number_;
	int linear_x_, linear_y_, linear_z_, angular_;
	double l_scale_, a_scale_, l_scale_z_;
	//! It will publish into command velocity (for the robot) and the ptz_state (for the pantilt)
	ros::Publisher vel_pub_, ptz_pub_;
	//! It will be suscribed to the joystick
	ros::Subscriber pad_sub_;
	//! Name of the topic where it will be publishing the velocity
	std::string cmd_topic_vel_;
	//! Name of the service where it will be modifying the digital outputs
	std::string cmd_service_io_;
	//! Name of the topic where it will be publishing the pant-tilt values
	std::string cmd_topic_ptz_;
	double current_vel;
	//! Pad type
	std::string pad_type_;
	//! Number of the DEADMAN button
	int dead_man_button_, bumper_override_button_;
	//! Number of the button for increase or decrease the speed max of the joystick
	int speed_up_button_, speed_down_button_;
	int button_output_1_, button_output_2_;
	int output_1_, output_2_;
	bool bOutput1, bOutput2;
	//! button to change kinematic mode
	int button_kinematic_mode_;
	//! kinematic mode
	int kinematic_mode_;
	//! Service to modify the kinematic mode
	ros::ServiceClient setKinematicMode;
	//! Name of the service to change the mode
	std::string cmd_set_mode_;
	//! button to start the homing service
	int button_home_;
	//! Service to start homing
	ros::ServiceClient doHome;
	//! Name of the service to do the homing
	std::string cmd_home_;
	//! buttons to the pan-tilt-zoom camera
	int ptz_tilt_up_, ptz_tilt_down_, ptz_pan_right_, ptz_pan_left_;
	int ptz_zoom_wide_, ptz_zoom_tele_;
	//! Service to modify the digital outputs
	ros::ServiceClient set_digital_outputs_client_;
	//! Service to safety module
	ros::ServiceClient  set_manual_release_client_, set_bumper_override_client_;
	//! Number of buttons of the joystick
	int num_of_buttons_;
	//! Pointer to a vector for controlling the event when pushing the buttons
	bool bRegisteredButtonEvent[DEFAULT_NUM_OF_BUTTONS];
        //! Pointer to a vector for controlling the event when pushing directional arrows (UNDER AXES ON PX4!)
        bool bRegisteredDirectionalArrows[4];

	// DIAGNOSTICS
	//! Diagnostic to control the frequency of the published command velocity topic
	diagnostic_updater::HeaderlessTopicDiagnostic *pub_command_freq;
	//! Diagnostic to control the reception frequency of the subscribed joy topic
	diagnostic_updater::HeaderlessTopicDiagnostic *sus_joy_freq;
	//! General status diagnostic updater
	diagnostic_updater::Updater updater_pad;
	//! Diagnostics min freq
	double min_freq_command, min_freq_joy; //
	//! Diagnostics max freq
	double max_freq_command, max_freq_joy; //
	//! Flag to enable/disable the communication with the publishers topics
	bool bEnable;
	//! Flag to track the first reading without the deadman's button pressed.
	bool last_command_;
	//! Client of the sound play service
	//  sound_play::SoundClient sc;
	//! Pan & tilt increment (degrees)
	double pan_increment_, tilt_increment_;
	//! Zoom increment (steps)
	int zoom_increment_;
	//! Add a dead zone to the joystick that controls scissor and robot rotation (only useful for xWam)
	std::string joystick_dead_zone_;
	//! Flag to enable the ptz control via axes
	bool ptz_control_by_axes_;
};


RBRoboutPad::RBRoboutPad():
  linear_x_(1),
  linear_y_(0),
  angular_(2),
  linear_z_(3),
  pnh_("~")
{
	current_vel = 0.1;

	//JOYSTICK PAD TYPE
	pnh_.param<std::string>("pad_type",pad_type_,"ps3");
	//
	pnh_.param("num_of_buttons", num_of_buttons_, DEFAULT_NUM_OF_BUTTONS);
	// MOTION CONF
	pnh_.param("axis_linear_x", linear_x_, DEFAULT_AXIS_LINEAR_X);
  	pnh_.param("axis_linear_y", linear_y_, DEFAULT_AXIS_LINEAR_Y);
	pnh_.param("axis_linear_z", linear_z_, DEFAULT_AXIS_LINEAR_Z);
	pnh_.param("axis_angular", angular_, DEFAULT_AXIS_ANGULAR);
	pnh_.param("scale_angular", a_scale_, DEFAULT_SCALE_ANGULAR);
	pnh_.param("scale_linear", l_scale_, DEFAULT_SCALE_LINEAR);
	pnh_.param("scale_linear_z", l_scale_z_, DEFAULT_SCALE_LINEAR_Z);
	pnh_.param("cmd_topic_vel", cmd_topic_vel_, cmd_topic_vel_);
	pnh_.param("button_dead_man", dead_man_button_, dead_man_button_);
	pnh_.param("button_bumber_override", bumper_override_button_, bumper_override_button_);
	pnh_.param("button_speed_up", speed_up_button_, speed_up_button_);  //4 Thrustmaster
	pnh_.param("button_speed_down", speed_down_button_, speed_down_button_); //5 Thrustmaster
	pnh_.param<std::string>("joystick_dead_zone", joystick_dead_zone_, "true");

	// DIGITAL OUTPUTS CONF
	pnh_.param("cmd_service_io", cmd_service_io_, cmd_service_io_);
	pnh_.param("button_output_1", button_output_1_, button_output_1_);
	pnh_.param("button_output_2", button_output_2_, button_output_2_);
	pnh_.param("output_1", output_1_, output_1_);
	pnh_.param("output_2", output_2_, output_2_);
	// PANTILT-ZOOM CONF
	pnh_.param("cmd_topic_ptz", cmd_topic_ptz_, cmd_topic_ptz_);
	pnh_.param("button_ptz_tilt_up", ptz_tilt_up_, ptz_tilt_up_);
	pnh_.param("button_ptz_tilt_down", ptz_tilt_down_, ptz_tilt_down_);
	pnh_.param("button_ptz_pan_right", ptz_pan_right_, ptz_pan_right_);
	pnh_.param("button_ptz_pan_left", ptz_pan_left_, ptz_pan_left_);
	pnh_.param("button_ptz_zoom_wide", ptz_zoom_wide_, ptz_zoom_wide_);
	pnh_.param("button_ptz_zoom_tele", ptz_zoom_tele_, ptz_zoom_tele_);
  	pnh_.param("button_home", button_home_, button_home_);
	pnh_.param("pan_increment",  pan_increment_, 0.09);
	pnh_.param("tilt_increment", tilt_increment_, 0.09);
	pnh_.param("zoom_increment", zoom_increment_, 200);

	// KINEMATIC MODE
	pnh_.param("button_kinematic_mode", button_kinematic_mode_, button_kinematic_mode_);
	pnh_.param("cmd_service_set_mode", cmd_set_mode_, cmd_set_mode_);
	pnh_.param("cmd_service_home", cmd_home_, std::string("home"));
	kinematic_mode_ = 1;

	ROS_INFO("RBRoboutPad num_of_buttons_ = %d, zoom = %d, %d", num_of_buttons_, ptz_zoom_wide_, ptz_zoom_tele_);
	for(int i = 0; i < num_of_buttons_; i++){
		bRegisteredButtonEvent[i] = false;
		ROS_INFO("bREG %d", i);
	}

	for(int i = 0; i < 3; i++){
	  bRegisteredDirectionalArrows[i] = false;
	}

	/*ROS_INFO("Service I/O = [%s]", cmd_service_io_.c_str());
	ROS_INFO("Topic PTZ = [%s]", cmd_topic_ptz_.c_str());
	ROS_INFO("Service I/O = [%s]", cmd_topic_vel_.c_str());
	ROS_INFO("Axis linear = %d", linear_);
	ROS_INFO("Axis angular = %d", angular_);
	ROS_INFO("Scale angular = %d", a_scale_);
	ROS_INFO("Deadman button = %d", dead_man_button_);
	ROS_INFO("OUTPUT1 button %d", button_output_1_);
	ROS_INFO("OUTPUT2 button %d", button_output_2_);
	ROS_INFO("OUTPUT1 button %d", button_output_1_);
	ROS_INFO("OUTPUT2 button %d", button_output_2_);*/

  	// Publish through the node handle Twist type messages to the guardian_controller/command topic
	vel_pub_ = nh_.advertise<geometry_msgs::Twist>(cmd_topic_vel_, 1);
	//  Publishes msgs for the pant-tilt cam
	ptz_pub_ = nh_.advertise<robotnik_msgs::ptz>(cmd_topic_ptz_, 1);

 	// Listen through the node handle sensor_msgs::Joy messages from joystick
	// (these are the references that we will sent to summit_xl_controller/command)
	pad_sub_ = nh_.subscribe<sensor_msgs::Joy>("joy", 10, &RBRoboutPad::padCallback, this);

 	// Request service to activate / deactivate digital I/O
	set_digital_outputs_client_ = nh_.serviceClient<robotnik_msgs::set_digital_output>(cmd_service_io_);
	set_manual_release_client_ = nh_.serviceClient<std_srvs::SetBool>("safety_module/set_manual_release");
	set_bumper_override_client_ = nh_.serviceClient<std_srvs::SetBool>("safety_module/set_bumper_override");
	bOutput1 = bOutput2 = false;

        // Request service to set kinematic mode
	setKinematicMode = nh_.serviceClient<robotnik_msgs::set_mode>(cmd_set_mode_);

	manual_release_false_number_  = 0;
	manual_release_true_number_   = 0;
	bumper_override_false_number_ = 0;
	bumper_override_true_number_  = 0;

        // Request service to start homing
	doHome = nh_.serviceClient<robotnik_msgs::home>(cmd_home_);

	// Diagnostics
	updater_pad.setHardwareID("None");
	// Topics freq control
	min_freq_command = min_freq_joy = 5.0;
	max_freq_command = max_freq_joy = 50.0;
	sus_joy_freq = new diagnostic_updater::HeaderlessTopicDiagnostic("/joy", updater_pad,
	                    diagnostic_updater::FrequencyStatusParam(&min_freq_joy, &max_freq_joy, 0.1, 10));

	pub_command_freq = new diagnostic_updater::HeaderlessTopicDiagnostic(cmd_topic_vel_.c_str(), updater_pad,
	                    diagnostic_updater::FrequencyStatusParam(&min_freq_command, &max_freq_command, 0.1, 10));


	bEnable = false;	// Communication flag disabled by default
	last_command_ = true;
	if(pad_type_=="ps4" || pad_type_=="logitechf710")
		ptz_control_by_axes_ = true;
	else
		ptz_control_by_axes_ = false;
}


/*
 *	\brief Updates the diagnostic component. Diagnostics
 *
 */
void RBRoboutPad::Update(){
	updater_pad.update();
}



void RBRoboutPad::padCallback(const sensor_msgs::Joy::ConstPtr& joy)
{
	geometry_msgs::Twist vel;
	robotnik_msgs::ptz ptz;
	bool ptzEvent = false;

	vel.linear.x = 0.0;
	vel.linear.y = 0.0;
	vel.linear.z = 0.0;

	vel.angular.x = 0.0;
	vel.angular.y = 0.0;
	vel.angular.z = 0.0;

	bEnable = (joy->buttons[dead_man_button_] == 1);

  	// Actions dependant on dead-man button
 	if (joy->buttons[dead_man_button_] == 1) {
		//ROS_ERROR("RBRoboutPad::padCallback: DEADMAN button %d", dead_man_button_);
		//Set the current velocity level

		// Safety module management
		manual_release_false_number_  = 0;
		// MANUAL RELEASE -> 1
		// write the signal X number of times
		if(manual_release_true_number_ < ITERATIONS_WRITE_MODBUS){
			setManualRelease(true);
			manual_release_true_number_++;
		}

		// L1 pressed -> Bumper override 1
		if(joy->buttons[bumper_override_button_] == 1){
			if(bumper_override_true_number_ < ITERATIONS_WRITE_MODBUS){
				setBumperOverride(true);
				bumper_override_true_number_++;
			}
			bumper_override_false_number_ = 0;
		}else{
			if(bumper_override_false_number_ < ITERATIONS_WRITE_MODBUS){
				setBumperOverride(false);
				bumper_override_false_number_++;
			}
			bumper_override_true_number_ = 0;
		}

		if ( joy->buttons[speed_down_button_] == 1 ){

			if(!bRegisteredButtonEvent[speed_down_button_])
				if(current_vel > 0.1){
		  			current_vel = current_vel - 0.1;
					bRegisteredButtonEvent[speed_down_button_] = true;
					ROS_INFO("Velocity: %f%%", current_vel*100.0);
					char buf[50]="\0";
 					int percent = (int) (current_vel*100.0);
					sprintf(buf," %d percent", percent);
                    // sc.say(buf);
				}
		}else{
			bRegisteredButtonEvent[speed_down_button_] = false;
		 }
		//ROS_ERROR("RBRoboutPad::padCallback: Passed SPEED DOWN button %d", speed_down_button_);
		if (joy->buttons[speed_up_button_] == 1){
			if(!bRegisteredButtonEvent[speed_up_button_])
				if(current_vel < 0.9){
					current_vel = current_vel + 0.1;
					bRegisteredButtonEvent[speed_up_button_] = true;
			 	 	ROS_INFO("Velocity: %f%%", current_vel*100.0);
  					char buf[50]="\0";
					int percent = (int) (current_vel*100.0);
					sprintf(buf," %d percent", percent);
                    // sc.say(buf);
				}

		}else{
			bRegisteredButtonEvent[speed_up_button_] = false;
		}
		//ROS_ERROR("RBRoboutPad::padCallback: Passed SPEED UP button %d", speed_up_button_);



		vel.linear.x = current_vel*l_scale_*joy->axes[linear_x_];
		if (kinematic_mode_ == 2) vel.linear.y = current_vel*l_scale_*joy->axes[linear_y_];
		else vel.linear.y = 0.0;

		//ROS_ERROR("RBRoboutPad::padCallback: Passed linear axes");

		if(joystick_dead_zone_=="true")
		{
			// limit scissor movement or robot turning (they are in the same joystick)
			if(joy->axes[angular_] == 1.0 || joy->axes[angular_] == -1.0) // if robot turning
			{
				// Same angular velocity for the three axis
				vel.angular.x = current_vel*(a_scale_*joy->axes[angular_]);
				vel.angular.y = current_vel*(a_scale_*joy->axes[angular_]);
				vel.angular.z = current_vel*(a_scale_*joy->axes[angular_]);

				vel.linear.z = 0.0;
			}
			else if (joy->axes[linear_z_] == 1.0 || joy->axes[linear_z_] == -1.0) // if scissor moving
			{
				vel.linear.z = current_vel*l_scale_z_*joy->axes[linear_z_]; // scissor movement

				// limit robot turn
				vel.angular.x = 0.0;
				vel.angular.y = 0.0;
				vel.angular.z = 0.0;
			}
			else
			{
				// Same angular velocity for the three axis
				vel.angular.x = current_vel*(a_scale_*joy->axes[angular_]);
				vel.angular.y = current_vel*(a_scale_*joy->axes[angular_]);
				vel.angular.z = current_vel*(a_scale_*joy->axes[angular_]);
				vel.linear.z = current_vel*l_scale_z_*joy->axes[linear_z_]; // scissor movement
			}
		}
		else // no dead zone
		{
			vel.angular.x = current_vel*(a_scale_*joy->axes[angular_]);
			vel.angular.y = current_vel*(a_scale_*joy->axes[angular_]);
			vel.angular.z = current_vel*(a_scale_*joy->axes[angular_]);
			vel.linear.z = current_vel*l_scale_z_*joy->axes[linear_z_];
		}

		//ROS_ERROR("RBRoboutPad::padCallback: Passed joystick deadzone ifelse");



		// LIGHTS
		if (joy->buttons[button_output_1_] == 1) {

			if(!bRegisteredButtonEvent[button_output_1_]){
				//ROS_INFO("RBRoboutPad::padCallback: OUTPUT1 button %d", button_output_1_);
				robotnik_msgs::set_digital_output write_do_srv;
				write_do_srv.request.output = output_1_;
				bOutput1=!bOutput1;
				write_do_srv.request.value = bOutput1;
				if(bEnable){
					set_digital_outputs_client_.call( write_do_srv );
					bRegisteredButtonEvent[button_output_1_] = true;
				}
			}
		}else{
			bRegisteredButtonEvent[button_output_1_] = false;
		}

		if (joy->buttons[button_output_2_] == 1) {

			if(!bRegisteredButtonEvent[button_output_2_]){
				//ROS_INFO("RBRoboutPad::padCallback: OUTPUT2 button %d", button_output_2_);
				robotnik_msgs::set_digital_output write_do_srv;
				write_do_srv.request.output = output_2_;
				bOutput2=!bOutput2;
				write_do_srv.request.value = bOutput2;

				if(bEnable){
					set_digital_outputs_client_.call( write_do_srv );
					bRegisteredButtonEvent[button_output_2_] = true;
				}
			}
		}else{
			bRegisteredButtonEvent[button_output_2_] = false;
		}

		//ROS_ERROR("RBRoboutPad::padCallback: Passed LIGHTS");

		// HOMING SERVICE
		if (joy->buttons[button_home_] == 1) {
			if (!bRegisteredButtonEvent[button_home_]) {
				robotnik_msgs::home home_srv;
				home_srv.request.request = true;
				if (bEnable) {
                                   ROS_INFO("RBRoboutPad::padCallback - Home");
   				   doHome.call( home_srv );
                                   bRegisteredButtonEvent[button_home_] = true;
                                   }
			}
			// Use this button also to block robot motion while moving the scissor
			vel.angular.x = 0.0;
	                vel.angular.y = 0.0;
        	        vel.angular.z = 0.0;
                	vel.linear.x = 0.0;
	                vel.linear.y = 0.0;
		}
		else {
			bRegisteredButtonEvent[button_home_]=false;
		}

		//ROS_ERROR("RBRoboutPad::padCallback: Passed HOME BUTTON");

		// PTZ
		ptz.pan = ptz.tilt = ptz.zoom = 0.0;
		ptz.relative = true;

		if(ptz_control_by_axes_)
		{
			if (joy->axes[ptz_tilt_up_] == -1.0) {
				if(!bRegisteredDirectionalArrows[AXIS_PTZ_TILT_UP]){
					 ptz.tilt = tilt_increment_;
					 //ROS_INFO("RBRoboutPad::padCallback: TILT UP");
					 bRegisteredDirectionalArrows[AXIS_PTZ_TILT_UP] = true;
					 ptzEvent = true;
				}
			}else{
				bRegisteredDirectionalArrows[AXIS_PTZ_TILT_UP] = false;
			}
			if (joy->axes[ptz_tilt_down_] == 1.0) {
				if(!bRegisteredDirectionalArrows[AXIS_PTZ_TILT_DOWN]){
					ptz.tilt = -tilt_increment_;
					//ROS_INFO("RBRoboutPad::padCallback: TILT DOWN");
					bRegisteredDirectionalArrows[AXIS_PTZ_TILT_DOWN] = true;
					ptzEvent = true;
				}
			}else{
				bRegisteredDirectionalArrows[AXIS_PTZ_TILT_DOWN] = false;
			}
			if (joy->axes[ptz_pan_left_] == -1.0) {
				if(!bRegisteredDirectionalArrows[AXIS_PTZ_PAN_LEFT]){
					ptz.pan = -pan_increment_;
					//ROS_INFO("RBRoboutPad::padCallback: PAN LEFT");
					bRegisteredDirectionalArrows[AXIS_PTZ_PAN_LEFT] = true;
					ptzEvent = true;
				}
			}else{
				bRegisteredDirectionalArrows[AXIS_PTZ_PAN_LEFT] = false;
			}
			if (joy->axes[ptz_pan_right_] == 1.0) {
				if(!bRegisteredDirectionalArrows[AXIS_PTZ_PAN_RIGHT]){
					ptz.pan = pan_increment_;
					//ROS_INFO("RBRoboutPad::padCallback: PAN RIGHT");
					bRegisteredDirectionalArrows[AXIS_PTZ_PAN_RIGHT] = true;
					ptzEvent = true;
				}
			}else{
				bRegisteredDirectionalArrows[AXIS_PTZ_PAN_RIGHT] = false;
			}

		}else{
		  //ROS_ERROR("RBRoboutPad::padCallback: INSIDE ELSE PTZ PAD_TYPE");
		  // TILT-MOVEMENTS (RELATIVE POS)
		  if (joy->buttons[ptz_tilt_up_] == 1) {
		    if(!bRegisteredButtonEvent[ptz_tilt_up_]){
		      ptz.tilt = tilt_increment_;
		      //ROS_INFO("RBRoboutPad::padCallback: TILT UP");
		      bRegisteredButtonEvent[ptz_tilt_up_] = true;
		      ptzEvent = true;
		    }
		  }else {
		    bRegisteredButtonEvent[ptz_tilt_up_] = false;
		  }

		  if (joy->buttons[ptz_tilt_down_] == 1) {
		    if(!bRegisteredButtonEvent[ptz_tilt_down_]){
		      ptz.tilt = -tilt_increment_;
		      //ROS_INFO("RBRoboutPad::padCallback: TILT DOWN");
		      bRegisteredButtonEvent[ptz_tilt_down_] = true;
		      ptzEvent = true;
		    }
		  }else{
		    bRegisteredButtonEvent[ptz_tilt_down_] = false;
		  }

		  // PAN-MOVEMENTS (RELATIVE POS)
		  if (joy->buttons[ptz_pan_left_] == 1) {
		    if(!bRegisteredButtonEvent[ptz_pan_left_]){
		      ptz.pan = -pan_increment_;
		      //ROS_INFO("RBRoboutPad::padCallback: PAN LEFT");
		      bRegisteredButtonEvent[ptz_pan_left_] = true;
		      ptzEvent = true;
		    }
		  }else{
		    bRegisteredButtonEvent[ptz_pan_left_] = false;
		  }

		  if (joy->buttons[ptz_pan_right_] == 1) {
		    if(!bRegisteredButtonEvent[ptz_pan_right_]){
		      ptz.pan = pan_increment_;
		      //ROS_INFO("RBRoboutPad::padCallback: PAN RIGHT");
		      bRegisteredButtonEvent[ptz_pan_right_] = true;
		      ptzEvent = true;
		    }
		  }else{
			bRegisteredButtonEvent[ptz_pan_right_] = false;
		  }
		}

		// ZOOM Settings (RELATIVE)
		if (joy->buttons[ptz_zoom_wide_] == 1) {
			if(!bRegisteredButtonEvent[ptz_zoom_wide_]){
				ptz.zoom = -zoom_increment_;
				//ROS_INFO("RBRoboutPad::padCallback: ZOOM WIDe");
				bRegisteredButtonEvent[ptz_zoom_wide_] = true;
				ptzEvent = true;
			}
		}else{
			bRegisteredButtonEvent[ptz_zoom_wide_] = false;
		}

		if (joy->buttons[ptz_zoom_tele_] == 1) {
			if(!bRegisteredButtonEvent[ptz_zoom_tele_]){
			  	ptz.zoom = zoom_increment_;
				//ROS_INFO("RBRoboutPad::padCallback: ZOOM TELE");
				bRegisteredButtonEvent[ptz_zoom_tele_] = true;
				ptzEvent = true;
			}
		}else{
			bRegisteredButtonEvent[ptz_zoom_tele_] = false;
		}

		if (joy->buttons[button_kinematic_mode_] == 1) {

			if(!bRegisteredButtonEvent[button_kinematic_mode_]){
				// Define mode (inc) - still coupled
				kinematic_mode_ += 1;
				if (kinematic_mode_ > 2) kinematic_mode_ = 1;
 				ROS_INFO("SummitXLJoy::joyCallback: Kinematic Mode %d ", kinematic_mode_);
				// Call service
				robotnik_msgs::set_mode set_mode_srv;
				set_mode_srv.request.mode = kinematic_mode_;
				setKinematicMode.call( set_mode_srv );
				bRegisteredButtonEvent[button_kinematic_mode_] = true;
			}
		}else{
			bRegisteredButtonEvent[button_kinematic_mode_] = false;
		}

		//ROS_ERROR("RBRoboutPad::padCallback: Passed SPHERE CAM and KINEMATIC MODE");

	}
   	else {

		// Safety module management
		// MANUAL RELEASE -> 0
		if(manual_release_false_number_ < ITERATIONS_WRITE_MODBUS){
			setManualRelease(false);
			manual_release_false_number_++;
		}

		if(bumper_override_false_number_ < ITERATIONS_WRITE_MODBUS){
			setBumperOverride(false);
			bumper_override_false_number_++;
		}

		manual_release_true_number_ = 0;
		bumper_override_true_number_ = 0;


		vel.angular.x = 0.0;	vel.angular.y = 0.0; vel.angular.z = 0.0;
		vel.linear.x = 0.0; vel.linear.y = 0.0; vel.linear.z = 0.0;
	}

	sus_joy_freq->tick();	// Ticks the reception of joy events

     // Publish
	// Only publishes if it's enabled
	if(bEnable){
		if (ptzEvent) ptz_pub_.publish(ptz);
		vel_pub_.publish(vel);
		pub_command_freq->tick();
		last_command_ = true;
		}


	if(!bEnable && last_command_){
		if (ptzEvent) ptz_pub_.publish(ptz);

		vel.angular.x = 0.0;  vel.angular.y = 0.0; vel.angular.z = 0.0;
		vel.linear.x = 0.0;   vel.linear.y = 0.0; vel.linear.z = 0.0;
		vel_pub_.publish(vel);
		pub_command_freq->tick();
		last_command_ = false;
		}
}

int RBRoboutPad::setManualRelease(bool value){
	std_srvs::SetBool set_bool_msg;

	set_bool_msg.request.data = value;
	set_manual_release_client_.call(set_bool_msg);

	return 0;
}

int RBRoboutPad::setBumperOverride(bool value){
	std_srvs::SetBool set_bool_msg;

	set_bool_msg.request.data = value;
	set_bumper_override_client_.call(set_bool_msg);

	return 0;
}

int main(int argc, char** argv)
{
	ros::init(argc, argv, "rbrobout_pad");
	RBRoboutPad rbrobout_pad;

	ros::Rate r(50.0);

	while( ros::ok() ){
		// UPDATING DIAGNOSTICS
		rbrobout_pad.Update();
		ros::spinOnce();
		r.sleep();
		}
}
