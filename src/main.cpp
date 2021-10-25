#include <mrs_msgs/UavState.h>
#include <mrs_msgs/VelocityReferenceStamped.h>
#include <visualization_msgs/Marker.h>
#include <vector>

#include <ros/ros.h>
#include <mutex>

#include "Vec3.h"
#include "World.h"
#include "Object.h"
#include "Node.h"
#include "RRT_tree.h"

bool ready = false;
mrs_msgs::UavState::ConstPtr uav_state;
std::mutex uav_state_mutex;


void odomCallback(const mrs_msgs::UavState::ConstPtr &msg);

int main(int argc, char **argv)
{
	ros::init(argc, argv, "drone_planner");

	// VELOCITY CONTROL
	size_t uav_id = 1;
	std::string vel_pub_topic  = "/uav" + std::to_string(uav_id) + "/control_manager/velocity_reference";
	std::string odom_sub_topic = "/uav" + std::to_string(uav_id) + "/odometry/uav_state";

	ros::NodeHandle n;
	ros::Subscriber odom_sub  = n.subscribe(odom_sub_topic, 100, odomCallback);
	ros::Publisher  vel_pub   = n.advertise<mrs_msgs::VelocityReferenceStamped>(vel_pub_topic, 100);

	ros::Rate rate(10);
	mrs_msgs::VelocityReferenceStamped cmd;
	std::vector<bool> direction_state{true, false, false, false}; // right, forward, left, forward
	
	cmd.reference.velocity.x = 0.5;
	cmd.reference.velocity.y = 0;
	
	// MARKER RVIZ
	ros::init(argc, argv, "basic_shapes");
	ros::NodeHandle markers_node_publisher;
	ros::Publisher vis_pub = markers_node_publisher.advertise<visualization_msgs::Marker>
	        ("visualization_marker", 10);

	World my_world;
	
	Vec3 pt_start(-3, 0, 0);
	Vec3 pt_goal(8.0, 0,0);

    Vec3 rock(2.0, -4.0, 0.0);
    my_world.add_obstacle(3.0, rock);
    Vec3 rock1(2.0, 4.0, 0.0);
    my_world.add_obstacle(3.5, rock1);
    Vec3 rock2(2.0, -4.0, 4.0);
    my_world.add_obstacle(3.0, rock2);
    Vec3 rock3(2.0, 4.0, 4.0);
    my_world.add_obstacle(3.5, rock3);
    Vec3 rock4(2.0, -4.0, -4.0);
    my_world.add_obstacle(3.0, rock4);
    Vec3 rock5(2.0, 4.0, -4.0);
    my_world.add_obstacle(3.5, rock5);

    Vec3 rock6(2.0, 0.0, 4.0);
    my_world.add_obstacle(3.0, rock6);
    Vec3 rock7(2.0, 0.0, -4.0);
    my_world.add_obstacle(3.5, rock7);

    Vec3 pt_medium(0.0, 0.0, 0.0);
    Vec3 pt_side(0.0, 5.0, 0.0);
    std::cout << Vec3::DoesLineSegmentIntersectSphere(pt_start, pt_goal, rock, 3.0) << "\n";
    std::cout << Vec3::DoesLineSegmentIntersectSphere(pt_start, pt_medium, rock, 3.0) << "\n";
    std::cout << Vec3::DoesLineSegmentIntersectSphere(pt_start, pt_side, rock, 3.0) << "\n";

    double goal_radius = 1.0;

	std::vector<Vec3> path = RRT_tree::find_path_to_goal(&my_world, pt_start, pt_goal, goal_radius);

	std::cout << "started in main:\n";
	for (const auto& point : path) {
        printf("%lf %lf %lf\n", point.x, point.y, point.z);
        my_world.add_object(0.2, point);
    }

    my_world.add_object(goal_radius, pt_goal);
    my_world.add_object(1.0, pt_start);
    auto k = my_world.objects.size();
    my_world.objects[k - 3].set_as_a_goal();
    my_world.objects[k - 2].set_as_a_goal();
    my_world.objects[k - 1].set_as_a_start();
    my_world.publish_world(vis_pub);
    World::publish_path(vis_pub, path);


//	Vec3 pt1(1.0, 0.0, 0.0);
//	Vec3 pt2;
//	Vec3 pt3 = pt1 + pt2;
	
//	int num = 15;
//	my_world.objects.reserve(num);
//
//	for (int i = 0; i < num; ++i) {
//		std::string s = "name" + std::to_string(i);
//		my_world.add_object(s, 0.3, Vec3::random_vec3(-2, 2));
//		my_world.objects[i].print_out_info();
//	}
//
//	my_world.add_object("Goal", 1, Vec3(2, 2, 2));
//	auto k = my_world.objects.size();
//	my_world.objects[k - 1].set_as_a_goal();

//	my_world.add_object("First", 0.6, Vec3(1, 0, 1));
//	my_world.add_object("Second", 1.8, Vec3(1.5, 0, 0));
//	my_world.add_object("Third", 2, Vec3(3, 0, 1));
//	auto k = my_world.objects.size();
//	my_world.objects[k - 1].set_as_a_goal();
	
//	std::cout << Object::are_intersecting(my_world.objects[k-1], my_world.objects[1]) << std::endl;
//	std::cout << Object::are_intersecting(my_world.objects[1], my_world.objects[0]) << std::endl;
	
//	Vec3 coords(0, 0, 0);
//	Node root = Node(coords);
//
//	coords = Vec3(1, 0, 0);
//	root.add_child(coords);
//
//	coords = Vec3(2, 0, 0);
//	root.add_child(coords);
//
//	coords = Vec3(3, 1, 5);
//	root.children[1]->add_child(coords);
//
//	root.print_out_all_children();
//	root.children[0]->print_out_all_children();
//	root.children[1]->print_out_all_children();
//
//	coords = Vec3(2, 1, 5);
//	auto closest = Node::find_the_closest_node(coords, &root);
//
//	std::cout << "Found closest: " << closest->coords.x <<  closest->coords.y << closest->coords.z << std::endl;

//	my_world.publish_world(vis_pub);
	
	while(ros::ok())
	{
		// get state
		mrs_msgs::UavState::ConstPtr cur_uav_state;
		if (ready)
		{
			std::lock_guard<std::mutex> lock(uav_state_mutex);
//			cur_uav_state = uav_state;
//			ROS_INFO("I heard: [%f]", cur_uav_state->pose.position.x);
		}
		
//		std::cout << cur_uav_state->pose.position.x << std::endl;
//		std::cout << "publishing: [" << ros::Time::now().toSec() << "] " << cmd.reference.velocity.x << ", " << cmd.reference.velocity.y  << std::endl;
//		vis_pub.publish(marker0);

		ros::spinOnce();
		rate.sleep();
	}
	return EXIT_SUCCESS;
}

//mrs_msgs::VelocityReferenceStamped &cmd,
//cmd.reference.velocity.y = 0;
//vel_pub.publish(cmd);

void odomCallback(mrs_msgs::UavState::ConstPtr const &msg)
{
	std::lock_guard<std::mutex> lock(uav_state_mutex);
	uav_state = msg;
	ready = true;
}
