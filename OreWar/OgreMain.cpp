#include <Ogre.h>
#include <OIS.h>
#include <OgreMath.h>
#include <sstream>
#include <vector>
#include "PhysicsEngine.h"
#include "GameObjects.h"
#include "RenderModel.h"
#include "OgreTextAreaOverlayElement.h"
#include "OgreFontManager.h"
#include "Gorilla.h"
 
using namespace Ogre;
 
class TestFrameListener : public FrameListener
{
public:
	TestFrameListener(OIS::Keyboard *keyboard, OIS::Mouse *mouse, SceneManager *mgr, Camera *cam, RenderWindow * renderWindow)
        : m_Keyboard(keyboard), m_mouse(mouse), m_rotateNode(mgr->getRootSceneNode()->createChildSceneNode()), m_cam(cam), 
		m_camHeight(0), m_camOffset(0), m_arena(200000, 2048, 10), m_mgr(mgr),
		m_thirdPersonCam(false), m_renderModel(m_arena, m_mgr, 2048, 10), mp_vp(cam->getViewport()), mp_fps(NULL), m_timer(0),
		mp_renderWindow(renderWindow), m_con(NULL), m_camParticle(NULL), m_camNode(NULL), m_camParticleNode(NULL),
		mp_healthBar(NULL), mp_energyBar(NULL), mp_speedBar(NULL), m_clearReleased(true)
	{
		m_cam->setFarClipDistance(0);
		m_arena.generateSolarSystem();

		// Generate the keyboard testing entity and attach it to the listener's scene node
		SpaceShip playerShip = SpaceShip(ObjectType::SHIP, 1, Vector3(20000, 40000, 20000), 15, m_arena.memoryManager());
		playerShip.addPlasmaCannon(PlasmaCannon(m_arena.memoryManager()));
		playerShip.addAnchorLauncher(AnchorLauncher(m_arena.memoryManager()));
		SpaceShip * p_playerShip = m_arena.setPlayerShip(playerShip);

		// Generate GUI elements
		Gorilla::Silverback * gorilla = Gorilla::Silverback::getSingletonPtr();
		Gorilla::Screen * screen = gorilla->createScreen(mp_vp, "dejavu");
		Gorilla::Layer * layer = screen->createLayer(10);

		layer->createCaption(14, 5, 5, "OreWar Alpha v0.04");
		Gorilla::Rectangle * crosshair = layer->createRectangle(Vector2(mp_vp->getActualWidth() / 2.0f - 6, mp_vp->getActualHeight() / 2.0f - 6),
			Vector2(12, 12));
		crosshair->background_image("crosshair");

		mp_fps = layer->createCaption(14, 5, mp_vp->getActualHeight() - 24, "FPS Counter");

		mp_healthBar = layer->createRectangle(Vector2(14, mp_vp->getActualHeight() - 66), Vector2(mp_vp->getActualWidth() * 0.25, 12));
		mp_healthBar->background_colour(Gorilla::Colours::Red);
		mp_healthBar->border_colour(Gorilla::Colours::Red);

		Gorilla::Rectangle * healthBarBorder = layer->createRectangle(Vector2(14, mp_vp->getActualHeight() - 66), Vector2(mp_vp->getActualWidth() * 0.25, 12));
		healthBarBorder->no_background();
		healthBarBorder->border_colour(Gorilla::Colours::Red);
		healthBarBorder->border_width(1);

		mp_energyBar = layer->createRectangle(Vector2(14, mp_vp->getActualHeight() - 82), Vector2(mp_vp->getActualWidth() * 0.25, 12));
		mp_energyBar->background_colour(Gorilla::Colours::Blue);
		mp_energyBar->border_colour(Gorilla::Colours::Blue);

		Gorilla::Rectangle * energyBarBorder = layer->createRectangle(Vector2(14, mp_vp->getActualHeight() - 82), Vector2(mp_vp->getActualWidth() * 0.25, 12));
		energyBarBorder->no_background();
		energyBarBorder->border_colour(Gorilla::Colours::Blue);
		energyBarBorder->border_width(1);

		mp_speedBar = layer->createRectangle(Vector2(14, mp_vp->getActualHeight() - 98), Vector2(mp_vp->getActualWidth() * 0.25, 12));
		mp_speedBar->background_colour(Gorilla::Colours::Orange);
		mp_energyBar->border_colour(Gorilla::Colours::Orange);

		m_camNode = m_mgr->getRootSceneNode()->createChildSceneNode();
		m_camParticleNode = m_mgr->getRootSceneNode()->createChildSceneNode();
		m_camNode->attachObject(m_cam);
		m_cam->setPosition(0, 0, 0);
		m_camParticle = m_mgr->createParticleSystem("CamStars", "Orewar/CamStarField");
		m_camParticle->setEmitting(true);
		m_camParticleNode->attachObject(m_camParticle);
    }
 
    bool frameStarted(const FrameEvent& evt)
    {
		// Capture the keyboard input
        m_Keyboard->capture();
		SpaceShip * playerShip = m_arena.playerShip();
		SphereCollisionObject * playerShipPhys = playerShip->phys();

		// Add random NPC ships to shoot
		for(int i = 0; i < m_arena.npcShips()->size() - 5; i++) {
			// Add some NPC ships
			SpaceShip npcShip = SpaceShip(ObjectType::NPC_SHIP, 1, 
				Vector3(Math::RangeRandom(20000, 50000),
				Math::RangeRandom(20000, 50000), 
				Math::RangeRandom(20000, 50000)),
				5,
				m_arena.memoryManager());
			SphereCollisionObject * npcShipPhysics = npcShip.phys();
			// npcShip.velocity(Vector3(0, 0, 0));
			npcShipPhysics->velocity(Vector3(Math::RangeRandom(0, 2000),
				Math::RangeRandom(0, 2000),
				Math::RangeRandom(0, 2000)));

			npcShipPhysics->orientation(Vector3(0, 0, -1).getRotationTo(npcShipPhysics->velocity()));
			m_arena.addNpcShip(npcShip);
		}

		if(m_Keyboard->isKeyDown(OIS::KC_G)) {
			if(m_clearReleased) {
				m_arena.clearSolarSystem();
				m_arena.generateSolarSystem();
				m_clearReleased = false;
			}
		} else {
			m_clearReleased = true;
		}

		// Adjust or reset the camera modifiers
		if(m_Keyboard->isKeyDown(OIS::KC_Z)) {
			m_camHeight = 0;
			m_camOffset = 0;
		}
		if(m_Keyboard->isKeyDown(OIS::KC_UP)) {
			m_camHeight += 2;
		}
		if(m_Keyboard->isKeyDown(OIS::KC_DOWN)) {
			m_camHeight -= 2;
		}
		if(m_Keyboard->isKeyDown(OIS::KC_RIGHT)) {
			m_camOffset += 2;
		}
		if(m_Keyboard->isKeyDown(OIS::KC_LEFT)) {
			m_camOffset -= 2;
		}

		// Mouse control
		m_mouse->capture();
		playerShipPhys->pitch(Radian(m_mouse->getMouseState().Y.rel * -0.25 * evt.timeSinceLastFrame));
		playerShipPhys->yaw(Radian(m_mouse->getMouseState().X.rel * -0.25 * evt.timeSinceLastFrame));
		
		// Clear all existing forces, and add keyboard forces
		Real energyDrain = 10;
		if(m_Keyboard->isKeyDown(OIS::KC_W)) {
			playerShipPhys->applyTempForce(playerShipPhys->heading() * Real(3000));
			// playerShip->drainEnergy(energyDrain * evt.timeSinceLastFrame);
		}
		if(m_Keyboard->isKeyDown(OIS::KC_S)) {
			playerShipPhys->applyTempForce(playerShipPhys->heading() * Real(-3000));
			// playerShip->drainEnergy(energyDrain * evt.timeSinceLastFrame);
		}
		if(m_Keyboard->isKeyDown(OIS::KC_A)) {
			playerShipPhys->applyTempForce((playerShipPhys->orientation() * Quaternion(Degree(90), Vector3::UNIT_Y)) 
				* Vector3(0, 0, -2000));
			// playerShip->drainEnergy(energyDrain * evt.timeSinceLastFrame);
		}
		if(m_Keyboard->isKeyDown(OIS::KC_D)) {
			playerShipPhys->applyTempForce((playerShipPhys->orientation() * Quaternion(Degree(-90), Vector3::UNIT_Y)) 
				* Vector3(0, 0, -2000));
			// playerShip->drainEnergy(energyDrain * evt.timeSinceLastFrame);
		}

		if(m_Keyboard->isKeyDown(OIS::KC_Q)) {
			playerShipPhys->roll(Radian(2 * evt.timeSinceLastFrame));
		}
		if(m_Keyboard->isKeyDown(OIS::KC_E)) {
			playerShipPhys->roll(Radian(-2 * evt.timeSinceLastFrame));
		}

		if(m_Keyboard->isKeyDown(OIS::KC_LCONTROL)) {
			playerShipPhys->applyTempForce(playerShipPhys->velocity().normalisedCopy() * (-1) * Vector3(2000, 2000, 2000));
			// playerShip->drainEnergy(energyDrain * evt.timeSinceLastFrame);
		}

		if(m_Keyboard->isKeyDown(OIS::KC_C)) {
			m_thirdPersonCam = !m_thirdPersonCam; 
		}

		// Generate projectile if required
		if(m_mouse->getMouseState().buttonDown(OIS::MB_Left)) {
			m_arena.fireProjectileFromShip(playerShip, 0);
		}

		if(m_mouse->getMouseState().buttonDown(OIS::MB_Right)) {
			m_arena.fireProjectileFromShip(playerShip, 1);
		}

		// Generate constraint - TESTING
		if(m_Keyboard->isKeyDown(OIS::KC_RCONTROL) || m_Keyboard->isKeyDown(OIS::KC_SPACE))
		{
			if(m_con == NULL) 
			{
				Projectile * closestAnchor = NULL;
				for(std::vector<Projectile * >::iterator projIter = m_arena.projectiles()->begin(); 
					projIter != m_arena.projectiles()->end();
					projIter++) 
				{
					if((*projIter)->type() != ANCHOR_PROJECTILE) {
						continue;
					}

					if(closestAnchor == NULL ||
						(playerShipPhys->position().squaredDistance((*projIter)->phys()->position()) <
						playerShipPhys->position().squaredDistance(closestAnchor->phys()->position()))) 
					{
						closestAnchor = *projIter;
					}
				}

				if(closestAnchor != NULL) {
					closestAnchor->phys()->velocity(Vector3(0, 0, 0));
					m_con = m_arena.addConstraint(Constraint(playerShip->phys(), 
						closestAnchor->phys(), false));
				}
			}
		} else {
			if(m_con != NULL) {
				PhysicsObject * deadAnchor = m_con->getTarget();
				m_arena.destroyConstraint(m_con);
				for(std::vector<Projectile * >::iterator projIter = m_arena.projectiles()->begin(); 
					projIter != m_arena.projectiles()->end();
					projIter++) 
				{
					if((*projIter)->phys() == deadAnchor) {
						m_arena.destroyProjectile(*projIter);
						break;
					}
				}
				m_con = NULL;
			}
		}

		// Update FPS counter
		m_timer += evt.timeSinceLastFrame;
		if (m_timer > 1.0f / 60.0f) 
		{
			m_timer = 0;
			mp_fps->text("FPS: " + Ogre::StringConverter::toString(mp_renderWindow->getLastFPS())
				// + " - RenderObjects: " + Ogre::StringConverter::toString(m_renderModel.getNumObjects())
				// + " - Health: " + Ogre::StringConverter::toString(playerShip->health())
				// + " - ModelMemPages: " + Ogre::StringConverter::toString(m_arena.memoryManager()->numPages())
				+ " - ModelAllocBytes: " + Ogre::StringConverter::toString(m_arena.memoryManager()->allocatedBytes())
				+ " - ModelTotalBytes: " + Ogre::StringConverter::toString(m_arena.memoryManager()->totalBytes())
				// + " - ModelCurPage: " + Ogre::StringConverter::toString(m_arena.memoryManager()->currentPage())
				// + " - RenderMemPages: " + Ogre::StringConverter::toString(m_renderModel.memoryManager()->numPages())
				+ " - RenderAllocBytes: " + Ogre::StringConverter::toString(m_renderModel.memoryManager()->allocatedBytes())
				+ " - RenderTotalBytes: " + Ogre::StringConverter::toString(m_renderModel.memoryManager()->totalBytes())
				// + " - RenderCurPage: " + Ogre::StringConverter::toString(m_renderModel.memoryManager()->currentPage())
				+ " - Speed: " + Ogre::StringConverter::toString(playerShipPhys->velocity().length())
				//+ " - Force: " + Ogre::StringConverter::toString((playerShipPhys->sumForces() + playerShipPhys->sumTempForces()).length())
				+ " - Normal: <" + Ogre::StringConverter::toString(playerShipPhys->normal().x)
				+ ", " + Ogre::StringConverter::toString(playerShipPhys->normal().y)
				+ ", " + Ogre::StringConverter::toString(playerShipPhys->normal().z) + ">");
		}

		// Update UI
		mp_healthBar->width((mp_vp->getActualWidth() * 0.25) * (playerShip->health() / playerShip->maxHealth()));
		mp_energyBar->width((mp_vp->getActualWidth() * 0.25) * (playerShip->energy() / playerShip->maxEnergy()));
		mp_speedBar->width((mp_vp->getActualWidth() * 0.25) * (playerShipPhys->velocity().length() / Real(6000)));

		// Update the position of the physics object and move the scene node
		m_arena.updatePhysics(evt.timeSinceLastFrame);
		m_renderModel.updateRenderList(evt.timeSinceLastFrame, m_camNode->getOrientation());

		// Move the camera
		if(m_thirdPersonCam) {
			m_camNode->setPosition(playerShipPhys->position() + Vector3(0, 1000, 1000));
			m_camNode->lookAt(playerShipPhys->position(), Node::TS_WORLD);
		} else {
			m_camNode->setPosition(playerShipPhys->position() + playerShipPhys->normal() * 80 - playerShipPhys->heading() * 200);
			m_camNode->setOrientation(playerShipPhys->orientation());
		}
		m_camParticleNode->setPosition(playerShipPhys->position() + playerShipPhys->velocity());

        return !m_Keyboard->isKeyDown(OIS::KC_ESCAPE);
    }
 
private:
    OIS::Keyboard *m_Keyboard;
	OIS::Mouse *m_mouse;
	SceneNode *m_rotateNode;
	Camera *m_cam;
	int m_camHeight;
	int m_camOffset;
	GameArena m_arena;
	bool m_thirdPersonCam;

	SceneManager * m_mgr;
	RenderModel m_renderModel;
	Viewport * mp_vp;
	Gorilla::Caption * mp_fps;
	Real m_timer;
	RenderWindow * mp_renderWindow;
	Constraint * m_con;
	ParticleSystem * m_camParticle;
	SceneNode * m_camNode;
	SceneNode * m_camParticleNode;

	Gorilla::Rectangle * mp_healthBar;
	Gorilla::Rectangle * mp_energyBar;
	Gorilla::Rectangle * mp_speedBar;

	bool m_clearReleased;
};
 
class Application
{
public:
    void go()
    {
        createRoot();
        defineResources();
        setupRenderSystem();
        createRenderWindow();
        initializeResourceGroups();
        setupScene();
        setupInputSystem();
        createFrameListener();
        startRenderLoop();
    }
 
    ~Application()
    {
		mInputManager->destroyInputObject(mKeyboard);
		OIS::InputManager::destroyInputSystem(mInputManager);
 		delete mListener;
        delete mRoot;
    }
 
private:
    Root *mRoot;
	SceneManager* mMgr;
    OIS::Keyboard * mKeyboard;
	OIS::Mouse * mMouse;
    OIS::InputManager *mInputManager;
    TestFrameListener *mListener;
 
    void createRoot()
    {
		// Generate the OGRE Root object
		#ifdef _DEBUG
			mRoot = new Root("plugins_d.cfg");
		#else
			mRoot = new Root("plugins.cfg");
		#endif
    }
 
    void defineResources()
    {
		String secName, typeName, archName;
        
		// Load the resources.cfg resource definition file
		ConfigFile cf;

		#ifdef _DEBUG
			cf.load("resources_d.cfg");
		#else
			cf.load("resources.cfg");
		#endif

		// Loop through all the provided resource folders and add them
		// to the ResourceGroupManager
		ConfigFile::SectionIterator seci = cf.getSectionIterator();
        while (seci.hasMoreElements())	{
			// Loop through the provided sections
			secName = seci.peekNextKey();
            ConfigFile::SettingsMultiMap *settings = seci.getNext();
            ConfigFile::SettingsMultiMap::iterator i;
	
			for (i = settings->begin(); i != settings->end(); ++i) {
				// Add each provided directory path to the GroupResourceManager
				typeName = i->first;
				archName = i->second;
				ResourceGroupManager::getSingleton().addResourceLocation(archName, typeName, secName);
            }
        }
    }
 
    void setupRenderSystem()
    {
		// Restore the last configuration settings, or provide a dialogue if no prior
		// settings are stored
		//if (!mRoot->restoreConfig() && !mRoot->showConfigDialog()) // Only show config if no existing config is found
		if (!mRoot->showConfigDialog()) // Always show config
			throw Exception(52, "User canceled the config dialog!", "Application::setupRenderSystem()");
    }
 
    void createRenderWindow()
    {
		// Create the render window (first parameter determines whether OGRE should generate
		// the render window)
		mRoot->initialise(true, "Ore War");
    }
 
    void initializeResourceGroups()
    {
		// Mipmaps setting must be set before resource groups are initialized
		TextureManager::getSingleton().setDefaultNumMipmaps(5);
        ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
    }
 
    void setupScene()
    {
		// Create the SceneManager
		SceneManager *mgr = mRoot->createSceneManager(ST_GENERIC, "Default SceneManager");

		// Initialize Gorilla
		Gorilla::Silverback * gorilla = new Gorilla::Silverback();
		gorilla->loadAtlas("dejavu");
		
		// Setup ambient light and shadows
		mgr->setShadowTechnique(Ogre::SHADOWTYPE_TEXTURE_ADDITIVE);
		mgr->setAmbientLight(Ogre::ColourValue(0.4, 0.4, 0.5));

		// Create a camera
		Camera *cam = mgr->createCamera("Camera");
		cam->setPosition(Ogre::Vector3(0,0,1000));
		cam->lookAt(Ogre::Vector3(0,0,0));

		// Create a viewport
        Viewport *vp = mRoot->getAutoCreatedWindow()->addViewport(cam);
		cam->setAspectRatio(Real(vp->getActualWidth()) / Real(vp->getActualHeight()));

		// Add a sky box
		mgr->setSkyBox(true, "Orewar/SpaceSkyBox", 40000, true);
    }
 
    void setupInputSystem()
    {
		// Initialize OIS
		size_t windowHnd = 0;
        std::ostringstream windowHndStr;
        OIS::ParamList pl;
        RenderWindow *win = mRoot->getAutoCreatedWindow();
 
        win->getCustomAttribute("WINDOW", &windowHnd);
        windowHndStr << windowHnd;
        pl.insert(std::make_pair(std::string("WINDOW"), windowHndStr.str()));
        mInputManager = OIS::InputManager::createInputSystem(pl);


		// Setup (Unbuffered in this case) input objects
		try {
            mKeyboard = static_cast<OIS::Keyboard*>(mInputManager->createInputObject(OIS::OISKeyboard, false));
            mMouse = static_cast<OIS::Mouse*>(mInputManager->createInputObject(OIS::OISMouse, false));
            //mJoy = static_cast<OIS::JoyStick*>(mInputManager->createInputObject(OIS::OISJoyStick, false));
        }
        catch (const OIS::Exception &e) {
            throw Exception(42, e.eText, "Application::setupInputSystem");
        }
    }
 
    void createFrameListener()
    {
		// Create and add a listener for the keyboard
		// Note: Input devices can have only one listener
		mListener = new TestFrameListener(mKeyboard, mMouse, mRoot->getSceneManager("Default SceneManager"), 
			mRoot->getSceneManager("Default SceneManager")->getCamera("Camera"),
			mRoot->getAutoCreatedWindow());
        mRoot->addFrameListener(mListener);
    }
 
    void startRenderLoop()
    {
		mRoot->startRendering();
    }
};

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#endif

#ifdef __cplusplus
	extern "C" {
#endif

#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
		INT WINAPI WinMain( HINSTANCE hInst, HINSTANCE, LPSTR strCmdLine, INT )
#else
		int main(int argc, char *argv[])
#endif
		{
			// Create application object
			Application app;

			try {
				app.go();
			} catch( Ogre::Exception& e ) {
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32
				MessageBox( NULL, e.getFullDescription().c_str(), "An exception has occured!", MB_OK | MB_ICONERROR | MB_TASKMODAL);
#else
				std::cerr << "An exception has occured: " <<
					e.getFullDescription().c_str() << std::endl;
#endif
			}

			return 0;
		}

#ifdef __cplusplus
	}
#endif