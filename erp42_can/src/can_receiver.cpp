#include "can_receiver.h"
#include "can_variables.h"

using namespace unmansol::erp42;

ERP42Receiver::ERP42Receiver():
  m_nh("~")
{
  Init_node();
}

void ERP42Receiver::Init_node()
{
  m_pub_feedback1 = m_nh.advertise<erp42_msgs::FeedBack1>("/erp42_can/feedback1",1);
  m_pub_feedback2 = m_nh.advertise<erp42_msgs::FeedBack2>("/erp42_can/feedback2",1);
  m_pub_test = m_nh.advertise<erp42_msgs::test>("/erp42_can/test",1);
}

void ERP42Receiver::Read()
{
  while ((this->m_RStatus=CAN_Read(PCAN_DEVICE, &m_RMessage, NULL)) == PCAN_ERROR_QRCVEMPTY)
    if (usleep(100))
      break;

  Update();

//  printf("  - R ID:%4x LEN:%1x DATA:%02x %02x %02x %02x %02x %02x %02x %02x\n",
//     (int)m_RMessage.ID, (int)m_RMessage.LEN,
//     (int)m_RMessage.DATA[0], (int)m_RMessage.DATA[1],
//     (int)m_RMessage.DATA[2], (int)m_RMessage.DATA[3],
//     (int)m_RMessage.DATA[4], (int)m_RMessage.DATA[5],
//     (int)m_RMessage.DATA[6], (int)m_RMessage.DATA[7]);

}

void ERP42Receiver::Update()
{
  switch (m_RMessage.ID)
  {
  case CAN_FEEDBACK_ID_1:
    std::cout << "feedback_1" << std::endl;

    m_feedback1_msg.MorA  = m_RMessage.DATA[0] & 0x01;
    m_feedback1_msg.EStop = m_RMessage.DATA[0] & 0x02;
    m_feedback1_msg.Gear  = m_RMessage.DATA[0] & 0x0c;
    m_feedback1_msg.speed = (m_RMessage.DATA[2] & 0xff) << 8  | (m_RMessage.DATA[1] & 0xff);
    m_feedback1_msg.steer = (m_RMessage.DATA[4] & 0xff) << 8  | (m_RMessage.DATA[3] & 0xff);;
    m_feedback1_msg.brake = m_RMessage.DATA[5];
    m_feedback1_msg.alive = m_RMessage.DATA[7];
    m_pub_feedback1.publish(m_feedback1_msg);
    break;

  case CAN_FEEDBACK_ID_2:
    std::cout << "feedback_2" << std::endl;

    m_feedback2_msg.encoder = (m_RMessage.DATA[3] & 0xff) << 24 |
                              (m_RMessage.DATA[2] & 0xff) << 16 |
                              (m_RMessage.DATA[1] & 0xff) << 8  |
                              (m_RMessage.DATA[0] & 0xff) ;
    m_feedback2_msg.brake_cmd_raw = m_RMessage.DATA[4];
    m_feedback2_msg.brake_raw = m_RMessage.DATA[5];
    m_feedback2_msg.brake_echo = m_RMessage.DATA[6];
    m_feedback2_msg.brake_init_max = m_RMessage.DATA[7];

    m_pub_feedback2.publish(m_feedback2_msg);


    break;

  case TEST_ID:
    m_test_msg.TestEncoder = m_RMessage.DATA[0] & 0xff |
           (m_RMessage.DATA[1] & 0xff) << 8;
    std::cout << "SAS DATA : "<<(int)m_test_msg.TestEncoder << std::endl;
    m_pub_test.publish(m_test_msg);
    break;

  default:
    std::cout << " None.. " << std::endl;
    break;
  }
}

bool ERP42Receiver::Connect()
{
  this->m_RStatus = CAN_Initialize(PCAN_DEVICE, PCAN_BAUD_500K, 0, 0, 0);

//  if (m_RStatus != PCAN_ERROR_OK)
//  {
//    char error_message[50];
//    CAN_GetErrorText(m_RStatus, 0x09, error_message);
//    ROS_ERROR("%s\n",error_message);
//  }

  if (this->m_RStatus) return false;
  else return true;
}


int main(int argc, char* argv[])
{
  ros::init(argc, argv, "can_receiver");

  ERP42Receiver erp_receiver;

  ros::Rate loop(50);

  while(ros::ok())
  {
    bool isConnect = erp_receiver.Connect();
    if (isConnect)
    {
      ROS_INFO(" Success Connection! ");
      break;
    }
    else ROS_ERROR(" Not Connected ");
    loop.sleep();
  }

  while(ros::ok())
  {
    ros::spinOnce();
    erp_receiver.Read();
  }

  return 0;
}
