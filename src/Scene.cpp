#include "Scene.hpp"

#include "irrlicht.h"

#include <iostream>
#include <vector>

namespace ic = irr::core;
namespace iv = irr::video;
namespace is = irr::scene;

//Constructor
Scene::Scene()
{
    // display values
    m_wind_speed      = 20;
    m_altitude        = 1000;
    m_vertical_speed  = -20;
    m_gauge_offset    = 0;
    m_stall          = true;

    m_planeSpeed    = 0.0f;
    m_planeAltitude = 0.0f;
    m_rotAngle      = 0.0f;

    // Render objects
    m_device = nullptr;
    m_smgr = nullptr;
    m_driver = nullptr;
    m_guiManager = nullptr;

    // Scene objects
    m_parentNode = nullptr;
    m_parentRotationNode = nullptr;
    m_city = nullptr;
    m_screw = nullptr;
    m_leftWing = nullptr;
    m_rightWing = nullptr;
    m_middleTail = nullptr;
    m_leftTail = nullptr;
    m_rightTail = nullptr;
    m_body = nullptr;
    m_gui = nullptr;
    m_water = nullptr;

    // Event objects
    m_receiver = nullptr;
}

void Scene::initializeIrrlicht()
{
    // Event manager
    m_receiver = new EventReceiver();

    // Window and rendering system creation
    m_device = createDevice(iv::EDT_OPENGL, ic::dimension2d<irr::u32>(640, 480), 16, false, false, false, m_receiver);
    m_driver = m_device->getVideoDriver();
    m_smgr  = m_device->getSceneManager();
    m_gui = m_device->getGUIEnvironment();
}

void Scene::manageCollisionsWithSurroundings(irr::scene::IMesh *city_mesh, irr::scene::ISceneNode* city_node)
{
    // Création du triangle selector
    is::ITriangleSelector *selector_city;
    selector_city = m_smgr->createOctreeTriangleSelector(city_mesh, city_node);
    city_node->setTriangleSelector(selector_city);
    // Et l'animateur/collisionneur
    is::ISceneNodeAnimator *anim_collision_plane_city;
    anim_collision_plane_city = m_smgr->createCollisionResponseAnimator(selector_city,
                                                 m_parentNode,  // Le noeud que l'on veut gérer
                                                 ic::vector3df(2.8, 0.5, 0.4), // "rayons" du perso
                                                 ic::vector3df(0, 0, 0),  // gravity
                                                 ic::vector3df(1.0,0,0));  //décalage du centre
    m_parentNode->addAnimator(anim_collision_plane_city);
}


void Scene::deleteData()
{
    delete m_city;
    delete m_body;
    delete m_screw;
    delete m_leftWing;
    delete m_rightWing;
    delete m_leftTail;
    delete m_rightTail;
    delete m_middleTail;
    delete m_water;
    delete m_receiver;
}

void Scene::initializeData()
{
    m_device->getFileSystem()->addFileArchive("data.zip");

    //City
    City* m_city = new City(m_smgr, "data/city/city_cercles.obj");
    m_city->initialize();

    //Init the object plane
    //2 parents: trajectory and rotation
    m_parentNode = m_smgr->addEmptySceneNode();
    m_parentRotationNode = m_smgr->addEmptySceneNode();
    m_parentRotationNode->setParent(m_parentNode);

    //Init the plane
    m_body = new Body(m_smgr, m_parentRotationNode, "data/plane/plane.obj");
    m_body->initialize();

    //Init the screw
    m_screw = new Screw(m_smgr, m_parentRotationNode, "data/plane/screw.obj");
    m_screw->initialize();

    //Init the two wings
    m_leftWing = new Wing(m_smgr, m_parentRotationNode,"data/plane/leftWing.obj");
    m_leftWing->setPosition(ic::vector3df(-0.667,0.303,0.19));
    m_leftWing->initialize();
    m_rightWing = new Wing(m_smgr, m_parentRotationNode,"data/plane/rightWing.obj");
    m_rightWing->setPosition(ic::vector3df(0.667,0.303,0.19));
    m_rightWing->initialize();

    //Init the three tails
    m_middleTail = new Tail(m_smgr, m_parentRotationNode, "data/plane/tail.obj");
    m_middleTail->setPosition(ic::vector3df(0.001,0.355,-0.53));
    m_middleTail->initialize();
    m_leftTail = new Tail(m_smgr, m_parentRotationNode, "data/plane/leftTail.obj");
    m_leftTail->setPosition(ic::vector3df(-0.205,0.23,-0.441));
    m_leftTail->initialize();
    m_rightTail = new Tail(m_smgr, m_parentRotationNode, "data/plane/rightTail.obj");
    m_rightTail->setPosition(ic::vector3df(0.208,0.225,-0.441));
    m_rightTail->initialize();

    //Water
    m_water = new Water(m_smgr, m_driver->getTexture("data/water/water.jpg"));
    m_water->initialize();

    // Collision management with surroundings
    manageCollisionsWithSurroundings(m_city->getMesh(), m_city->getNode());


}

void Scene::render()
{
    //If the plane is flying then
    //  inFlight = true
    //Else, ie. plane on the ground, in take-off position and in landing position
    //  inFlight = false
    ic::vector3df rotation = m_parentNode->getRotation();
    ic::vector3df position = m_parentNode->getPosition();

    if(m_receiver->getOnFloor())
    {
        m_receiver->planeOnFloor(m_parentRotationNode);

        rotation.Y      = m_receiver->getRotation();
        m_planeSpeed      = m_receiver->getSpeed();

        position.X += m_planeSpeed * sin(rotation.Y * M_PI / 180.0);
        position.Z += m_planeSpeed * cos(rotation.Y * M_PI / 180.0);
    }
    else if(m_receiver->getInTakeOff())
    {
        //std::cout<<"TD : plane is taking off"<<std::endl;
    }
    else if(m_receiver->getInFlight())
    {
        // Update screw rotation
        m_screw->updateRotation();

        m_receiver->planeInFlight(m_parentRotationNode, m_leftWing->getNode(), m_rightWing->getNode(), m_middleTail->getNode(), m_leftTail->getNode(), m_rightTail->getNode());


        rotation.Y      = m_receiver->getRotation();
        m_planeSpeed      = m_receiver->getSpeed();
        m_planeAltitude   = m_receiver->getAltitude();

        position.X += m_planeSpeed * sin(rotation.Y * M_PI / 180.0);
        position.Z += m_planeSpeed * cos(rotation.Y * M_PI / 180.0);
        position.Y  = m_planeAltitude;
    }
    else if(m_receiver->getInLanding())
    {
        //std::cout<<"TD : plane is landing"<<std::endl;
    }
    else if(m_receiver->getIsStalling())
    {
        //std::cout<<"TD : the plane is stalling"<<std::endl;
    }
    else
    {
        //std::cout<<"TD : the plane has crashed"<<std::endl;
    }

    m_parentNode->setRotation(rotation);
    m_parentNode->setPosition(position);

    //Camera position
    m_smgr->addCameraSceneNode(m_body->getNode(), ic::vector3df(0, 5, -34), m_parentNode->getPosition()); //0,5,-34

    //Back color
    m_driver->beginScene(true,true,iv::SColor(100,150,200,255));

    // Draw the scene
    m_smgr->drawAll();
    m_gui->drawAll();

    m_driver->endScene();
}


