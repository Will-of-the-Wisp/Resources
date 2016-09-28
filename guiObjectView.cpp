//-----------------------------------------------------------------------------
// Copyright (c) 2012 GarageGames, LLC
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#include "T3D/guiObjectView.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightManager.h"
#include "lighting/lightInfo.h"
#include "core/resourceManager.h"
#include "scene/sceneManager.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "math/mathTypes.h"
#include "gfx/gfxTransformSaver.h"
#include "console/engineAPI.h"


IMPLEMENT_CONOBJECT( GuiObjectView );

ConsoleDocClass( GuiObjectView,
   "@brief GUI control which displays a 3D model.\n\n"

   "Model displayed in the control can have other objects mounted onto it, and the light settings can be adjusted.\n\n"

   "@tsexample\n"
	"	new GuiObjectView(ObjectPreview)\n"
	"	{\n"
	"		shapeFile = \"art/shapes/items/kit/healthkit.dts\";\n"
	"		mountedNode = \"mount0\";\n"
	"		lightColor = \"1 1 1 1\";\n"
	"		lightAmbient = \"0.5 0.5 0.5 1\";\n"
	"		lightDirection = \"0 0.707 -0.707\";\n"
	"		orbitDiststance = \"2\";\n"
	"		minOrbitDiststance = \"0.917688\";\n"
	"		maxOrbitDiststance = \"5\";\n"
	"		cameraSpeed = \"0.01\";\n"
	"		cameraZRot = \"0\";\n"
	"		forceFOV = \"0\";\n"
	"		reflectPriority = \"0\";\n"
	"	};\n"
   "@endtsexample\n\n"

   "@see GuiControl\n\n"

   "@ingroup Gui3D\n"
);

IMPLEMENT_CALLBACK( GuiObjectView, onMouseEnter, void, (),(),
   "@brief Called whenever the mouse enters the control.\n\n"
   "@tsexample\n"
   "// The mouse has entered the control, causing the callback to occur\n"
   "GuiObjectView::onMouseEnter(%this)\n"
   "	{\n"
   "		// Code to run when the mouse enters this control\n"
   "	}\n"
   "@endtsexample\n\n"
   "@see GuiControl\n\n"
);

IMPLEMENT_CALLBACK( GuiObjectView, onMouseLeave, void, (),(),
   "@brief Called whenever the mouse leaves the control.\n\n"
   "@tsexample\n"
   "// The mouse has left the control, causing the callback to occur\n"
   "GuiObjectView::onMouseLeave(%this)\n"
   "	{\n"
   "		// Code to run when the mouse leaves this control\n"
   "	}\n"
   "@endtsexample\n\n"
   "@see GuiControl\n\n"
);

//------------------------------------------------------------------------------

GuiObjectView::GuiObjectView()
   :  mMaxOrbitDist( 5.0f ),
      mMinOrbitDist( 1.0f ), // <-- added( Restrict cam movement. ) --
      mOrbitDist( 5.0f ),
      mMouseState( None ),
      mModel( NULL ),
      mMountedModel( NULL ),
      mLastMousePoint( 0, 0 ),
      mLastRenderTime( 0 ),
      mRunThread( NULL ),
      mLight( NULL ),
      mAnimationSeq( -1 ),
      mMountNodeName( "mount0" ),
      mMountNode( -1 ),
	  mBaseNode( -1 ), // <- added( GuiObjectView extended Camera ) --
	  mEyeNode( -1 ), // <- added( GuiObjectView extended Camera ) --
	  mUseNodes( 0 ), // <- added( GuiObjectView extended Camera ) --
      mCameraSpeed( 0.01f ),
	  mCameraRotation( 0.0f, 0.0f, 0.0f ),
      mLightColor( 1.0f, 1.0f, 1.0f ),
      mLightAmbient( 0.5f, 0.5f, 0.5f ),
      mLightDirection( 0.f, 0.707f, -0.707f )
{
   mCameraMatrix.identity();
   mCameraRot.set( 0.0f, 0.0f, 0.0f );
   mCameraPos.set( 0.0f, 0.0f, 0.0f );
   mCameraMatrix.setColumn( 3, mCameraPos );
   mOrbitPos.set( 0.0f, 0.0f, 0.0f );

   // By default don't do dynamic reflection
   // updates for this viewport.
   mReflectPriority = 0.0f;
}

//------------------------------------------------------------------------------

GuiObjectView::~GuiObjectView()
{
   if( mModel )
      SAFE_DELETE( mModel );
   if( mMountedModel )
      SAFE_DELETE( mMountedModel );
   if( mLight )
      SAFE_DELETE( mLight );
}

//------------------------------------------------------------------------------

void GuiObjectView::initPersistFields()
{
   addGroup( "Model" );
   
      addField( "shapeFile", TypeStringFilename, Offset( mModelName, GuiObjectView ),
         "The object model shape file to show in the view." );
	  addProtectedField("skin", TypeRealString, Offset(mAppliedSkinName, GuiObjectView), &_setFieldSkin, &_getFieldSkin,
		  "@brief The skin applied to the shape.\n\n"); // <-- updated( GuiObjectView sub-mesh Skinning ) --
   
   endGroup( "Model" );
   
   addGroup( "Animation" );
   
      addField( "animSequence", TypeRealString, Offset( mAnimationSeqName, GuiObjectView ),
         "The animation sequence to play on the model." );

   endGroup( "Animation" );
   
   addGroup( "Mounting" );
   
      addField( "mountedShapeFile", TypeStringFilename, Offset( mMountedModelName, GuiObjectView ),
         "Optional shape file to mount on the primary model (e.g. weapon)." );
      addField( "mountedSkin", TypeRealString, Offset( mMountSkinName, GuiObjectView ),
         "Skin name used on mounted shape file." );
      addField( "mountedNode", TypeRealString, Offset( mMountNodeName, GuiObjectView ),
         "Name of node on primary model to which to mount the secondary shape." );
   
   endGroup( "Mounting" );
   
   addGroup( "Lighting" );
   
      addField( "lightColor", TypeColorF, Offset( mLightColor, GuiObjectView ),
         "Diffuse color of the sunlight used to render the model." );
      addField( "lightAmbient", TypeColorF, Offset( mLightAmbient, GuiObjectView ),
         "Ambient color of the sunlight used to render the model." );
      addField( "lightDirection", TypePoint3F, Offset( mLightDirection, GuiObjectView ),
         "Direction from which the model is illuminated." );
   
   endGroup( "Lighting" );
   
   addGroup( "Camera" );
      addField("useNodes", TypeBool, Offset(mUseNodes, GuiObjectView),
	     "Uses the shape's start01 node to set the camera position.");
      addField( "orbitDistance", TypeF32, Offset( mOrbitDist, GuiObjectView ),
         "Distance from which to render the model." );
      addField( "minOrbitDistance", TypeF32, Offset( mMinOrbitDist, GuiObjectView ),
         "Minumum distance to which the camera can be zoomed in." );
      addField( "maxOrbitDistance", TypeF32, Offset( mMaxOrbitDist, GuiObjectView ),
         "Maxiumum distance to which the camera can be zoomed out." );
      addField( "cameraSpeed", TypeF32, Offset( mCameraSpeed, GuiObjectView ),
         "Multiplier for mouse camera operations." );
      addField( "cameraRotation", TypePoint3F, Offset( mCameraRotation, GuiObjectView ),
         "Set the camera rotation." );
   endGroup( "Camera" );
   
   Parent::initPersistFields();
}

// -- added( GuiObjectView sub-mesh Skinning ) -->
bool GuiObjectView::_setFieldSkin(void *object, const char *index, const char *data)
{
	GuiObjectView* shape = static_cast<GuiObjectView*>(object);
	if (shape)
		shape->setSkinName(data);
	return false;
}

const char *GuiObjectView::_getFieldSkin(void *object, const char *data)
{
	GuiObjectView* shape = static_cast<GuiObjectView*>(object);
	return shape ? shape->getSkinName() : "";
}
// <-- added( GuiObjectView sub-mesh Skinning ) --

//------------------------------------------------------------------------------

void GuiObjectView::onStaticModified( StringTableEntry slotName, const char* newValue )
{
   Parent::onStaticModified( slotName, newValue );
   
   static StringTableEntry sShapeFile = StringTable->insert( "shapeFile" );
   static StringTableEntry sSkin = StringTable->insert( "skin" );
   static StringTableEntry sMountedShapeFile = StringTable->insert( "mountedShapeFile" );
   static StringTableEntry sMountedSkin = StringTable->insert( "mountedSkin" );
   static StringTableEntry sMountedNode = StringTable->insert( "mountedNode" );
   static StringTableEntry sLightColor = StringTable->insert( "lightColor" );
   static StringTableEntry sLightAmbient = StringTable->insert( "lightAmbient" );
   static StringTableEntry sLightDirection = StringTable->insert( "lightDirection" );
   static StringTableEntry sUseNodes = StringTable->insert( "useNodes" ); // <-- added( GuiObjectView extended Camera ) --
   static StringTableEntry sOrbitDistance = StringTable->insert( "orbitDistance" );
   static StringTableEntry sMinOrbitDistance = StringTable->insert( "minOrbitDistance" );
   static StringTableEntry sMaxOrbitDistance = StringTable->insert( "maxOrbitDistance" );
   static StringTableEntry sCameraRotation = StringTable->insert( "cameraRotation" );
   static StringTableEntry sAnimSequence = StringTable->insert( "animSequence" );
   
   if( slotName == sShapeFile )
      setObjectModel( String( mModelName ) );
   else if( slotName == sSkin )
	   setSkinName(String(mAppliedSkinName)); // <-- updated( GuiObjectView sub-mesh Skinning ) --
   else if( slotName == sMountedShapeFile )
      setMountedObject( String( mMountedModelName ) );
   else if( slotName == sMountedSkin )
      setMountSkin( String( mMountSkinName ) );
   else if( slotName == sMountedNode )
      setMountNode( String( mMountNodeName ) );
   else if( slotName == sLightColor )
      setLightColor( mLightColor );
   else if( slotName == sLightAmbient )
      setLightAmbient( mLightAmbient );
   else if (slotName == sLightDirection)
	  setLightDirection(mLightDirection);
   else if (slotName == sUseNodes) // <-- added( GuiObjectView extended Camera ) --
	  setUseNodes(mUseNodes); // <-- added( GuiObjectView extended Camera ) --
   else if( slotName == sOrbitDistance || slotName == sMinOrbitDistance || slotName == sMaxOrbitDistance )
      setOrbitDistance( mOrbitDist );
   else if( slotName == sCameraRotation )
      setCameraRotation( mCameraRotation );
   else if( slotName == sAnimSequence )
      setObjectAnimation( String( mAnimationSeqName ) );
}

//------------------------------------------------------------------------------

bool GuiObjectView::onWake()
{
   if( !Parent::onWake() )
      return false;

   if( !mLight )
   {
      mLight = LIGHTMGR->createLightInfo();   

      mLight->setColor( mLightColor );
      mLight->setAmbient( mLightAmbient );
      mLight->setDirection( mLightDirection );
   }

   // -- added( GuiObjectView Sub-Mesh Support ) -->
   if (*mModelName && mModelName[0]) 
   {
	   // Set the initial mesh hidden state.
	   mMeshHidden.setSize(mModel->getShape()->objects.size());
	   mMeshHidden.clear();
   }
   // <-- added( GuiObjectView Sub-Mesh Support ) --
   
   setUseNodes(mUseNodes); // <-- added( GuiObjectView Extended Camera ) --

   return true;
}

//------------------------------------------------------------------------------

void GuiObjectView::onMouseDown( const GuiEvent &event )
{
   if( !mActive || !mVisible || !mAwake )
      return;

   mMouseState = Rotating;
   mLastMousePoint = event.mousePoint;
   mouseLock();
}

//------------------------------------------------------------------------------

void GuiObjectView::onMouseUp( const GuiEvent &event )
{
   mouseUnlock();
   mMouseState = None;
}

//------------------------------------------------------------------------------

void GuiObjectView::onMouseDragged( const GuiEvent &event )
{
   if( mMouseState != Rotating )
      return;

   Point2I delta = event.mousePoint - mLastMousePoint;
   mLastMousePoint = event.mousePoint;

   // mCameraRot.x += ( delta.y * mCameraSpeed ); // <-- removed ( Restrict cam movement. )
   mCameraRot.z += ( delta.x * mCameraSpeed );
}

//------------------------------------------------------------------------------
// -- added( GuiObjectView Sub-mesh Support ) -->
void GuiObjectView::_updateHiddenMeshes()
{
	if (!mModel)
		return;

	// This may happen at some point in the future... lets
	// detect it so that it can be fixed at that time.
	AssertFatal(mMeshHidden.getSize() == mModel->mMeshObjects.size(),
		"ShapeBase::_updateMeshVisibility() - Mesh visibility size mismatch!");

	for (U32 i = 0; i < mMeshHidden.getSize(); i++)
		setMeshHidden(i, mMeshHidden.test(i));
}

void GuiObjectView::setMeshHidden(const char *meshName, bool forceHidden)
{
	setMeshHidden(mModel->getShape()->findObject(meshName), forceHidden);
}

void GuiObjectView::setMeshHidden(S32 meshIndex, bool forceHidden)
{
	if (meshIndex == -1 || meshIndex >= mMeshHidden.getSize())
		return;

	if (forceHidden)
		mMeshHidden.set(meshIndex);
	else
		mMeshHidden.clear(meshIndex);

	if (mModel)
		mModel->setMeshForceHidden(meshIndex, forceHidden);
}

void GuiObjectView::setAllMeshesHidden(bool forceHidden)
{
	if (forceHidden)
		mMeshHidden.set();
	else
		mMeshHidden.clear();

	if (mModel)
	{
		for (U32 i = 0; i < mMeshHidden.getSize(); i++)
			mModel->setMeshForceHidden(i, forceHidden);
	}
}

DefineEngineMethod(GuiObjectView, setAllMeshesHidden, void, (bool hidden), ,
	"@brief Set the hidden state on all the shape meshes.\n\n"

	"This allows you to hide all meshes in the shape, for example, and then only "
	"enable a few.\n"

	"@param hide new hidden state for all meshes\n\n")
{
	object->setAllMeshesHidden(hidden);
}

DefineEngineMethod(GuiObjectView, setMeshHidden, void, (const char* name, bool hidden), ,
	"@brief Set the hidden state on the named shape mesh.\n\n"

	"@param name name of the mesh to hide/show\n"
	"@param hide new hidden state for the mesh\n\n")
{
	object->setMeshHidden(name, hidden);
}
// <-- added( GuiObjectView sub-mesh Support ) --

//------------------------------------------------------------------------------

void GuiObjectView::onRightMouseDown( const GuiEvent &event )
{
   mMouseState = Zooming;
   mLastMousePoint = event.mousePoint;
   mouseLock();
}

//------------------------------------------------------------------------------

void GuiObjectView::onRightMouseUp( const GuiEvent &event )
{
   mouseUnlock();
   mMouseState = None;
}

//------------------------------------------------------------------------------

void GuiObjectView::onRightMouseDragged( const GuiEvent &event )
{
   if( mMouseState != Zooming )
      return;

   S32 delta = event.mousePoint.y - mLastMousePoint.y;
   mLastMousePoint = event.mousePoint;

   // -- added( GuiObjectView Extended Camera ) -->
   // Apply constraints to the zoom.
   F32 newDist = mOrbitDist + (delta * mCameraSpeed);
   if (newDist >= mMinOrbitDist && newDist <= mMaxOrbitDist)
	   mOrbitDist = newDist;

   mBaseNode = mModel->getShape()->findNode("start01");

   // If there is a 'start01' node, use its transform.
   if (mBaseNode != -1)
   {
	   // Get the transform of the 'start01' node.
	   MatrixF baseMat;
	   mModel->getShape()->getNodeWorldTransform(mBaseNode, &baseMat);

	   // Get the position of the 'start01' node.
	   Point3F baseXYZ;
	   baseMat.getColumn(3, &baseXYZ);
	   F32 baseZ = baseXYZ.z;

	   // Init the 'eye' node's index value.
	   mEyeNode = mModel->getShape()->findNode("eye");

	   // If there's an 'eye' node, get its z position.
	   if (mEyeNode != -1)
	   {
		   MatrixF eyeMat;
		   mModel->getShape()->getNodeWorldTransform(mEyeNode, &eyeMat);
		   Point3F eyeXYZ;
		   eyeMat.getColumn(3, &eyeXYZ);
		   F32 eyeZ = eyeXYZ.z;

		   // Calculate a z location near chest level.
		   F32 d = eyeZ - baseZ;
		   if (d < 1.5f)
		   {
			   F32 chestZ = d * 0.45f;
			   F32 newZ = mOrbitPos.z - (delta * mCameraSpeed) / mOrbitDist;
			   if (newZ > 1.0f && newZ < chestZ)
				   mOrbitPos.z = newZ;
		   }
		   else
		   {
			   F32 chestZ = d * 0.75f;
			   F32 newZ = mOrbitPos.z - (delta * mCameraSpeed) / mOrbitDist;
			   if (newZ > 1.0f && newZ < chestZ)
				   mOrbitPos.z = newZ;
		   }
	   }
   }
}

void GuiObjectView::setCamPos(Point3F xyz)
{
	mCameraPos.set(xyz);
}

void GuiObjectView::setOrbitPos(Point3F xyz)
{
	mOrbitPos.set(xyz);
}

F32 GuiObjectView::getEyeZ()
{
	// Get the z position of the 'eye' node.
	MatrixF eyeMat;
	mModel->getShape()->getNodeWorldTransform(mEyeNode, &eyeMat);
	Point3F eyeXYZ;
	eyeMat.getColumn(3, &eyeXYZ);
	F32 eyeZ = eyeXYZ.z;

	return eyeZ;
}

DefineEngineMethod(GuiObjectView, getCamPos, Point3F, (), ,
	"@brief Get the camera's position.\n\n")
{
	return object->getCamPos();
}

DefineEngineMethod(GuiObjectView, setCamPos, void, (Point3F xyz), ,
	"@brief Set the camera's position.\n\n")
{
	object->setCamPos(xyz);
}

DefineEngineMethod(GuiObjectView, getOrbitPos, Point3F, (), ,
	"@brief Get the camera's orbit position.\n\n")
{
	return object->getOrbitPos();
}

DefineEngineMethod(GuiObjectView, setOrbitPos, void, (Point3F xyz), ,
	"@brief Set the camera's orbit position.\n\n")
{
	return object->setOrbitPos(xyz);
}

DefineEngineMethod(GuiObjectView, getEyeZ, F32, (), ,
	"@brief Get the z position from the eye node.\n\n")
{
	return object->getEyeZ();
}
// <-- added( GuiObjectView Extended Camera ) --

//------------------------------------------------------------------------------

void GuiObjectView::setObjectAnimation( S32 index )
{
   mAnimationSeq = index;
   mAnimationSeqName = String();
   
   if( mModel )
      _initAnimation();
}

//------------------------------------------------------------------------------

void GuiObjectView::setObjectAnimation( const String& sequenceName )
{
   mAnimationSeq = -1;
   mAnimationSeqName = sequenceName;
   
   if( mModel )
      _initAnimation();
}

//------------------------------------------------------------------------------

void GuiObjectView::setObjectModel( const String& modelName )
{
   SAFE_DELETE( mModel );
   mRunThread = 0;
   mModelName = String::EmptyString;
   
   // Load the shape.

   Resource< TSShape > model = ResourceManager::get().load( modelName );
   if( !model )
   {
      Con::warnf( "GuiObjectView::setObjectModel - Failed to load model '%s'", modelName.c_str() );
      return;
   }
   
   // Instantiate it.

   mModel = new TSShapeInstance( model, true );
   mModelName = modelName;
   
   // -- added( GuiObjectView sub-meshes ) -->
   if (*mModelName && mModelName[0]) 
   {
	   // Set the initial mesh hidden state.
	   mMeshHidden.setSize(mModel->getShape()->objects.size());
	   mMeshHidden.clear();
   }

   reSkin();
   // <-- added( GuiObjectView sub-meshes ) --

   // -- added( GuiObjectView extended Camera ) -->
   // If the GuiObjectView is set to use nodes, initialize the node index values.
   if (mUseNodes)
   {
	   mBaseNode = mModel->getShape()->findNode("start01");

	   // If there is a 'start01' node, use its transform. Offset z by 1.
	   if (mBaseNode != -1)
	   {
		   // Get the transform of the 'start01' node.
		   MatrixF baseMat;
		   mModel->getShape()->getNodeWorldTransform(mBaseNode, &baseMat);

		   // Get the position of the 'start01' node.
		   Point3F baseXYZ;
		   baseMat.getColumn(3, &baseXYZ);

		   //Offset it up 1.
		   Point3F offset(0.0f, 0.0f, 1.0f);
		   mHomePos = baseXYZ + offset; // Store the home position for later use.
		   mHomeZ = mHomePos.z;
	   }
	   else
	   {
		   mOrbitPos = mModel->getShape()->center;
		   mMinOrbitDist = mModel->getShape()->radius;
	   }
   }
   else
   {
	   mOrbitPos = mModel->getShape()->center;
	   mMinOrbitDist = mModel->getShape()->radius;
   }
   // <-- added( GuiObjectView extended Camera Support ) --

   // Initialize camera values.
   
   mOrbitPos = mModel->getShape()->center;
   mMinOrbitDist = mModel->getShape()->radius;

   // Initialize animation.
   
   _initAnimation();
   _initMount();
}

// -- added( GuiObjectView sub-mesh Skinning ) --> 
// Set the mSkinNameHandle string and call to reskin the model.
void GuiObjectView::setSkinName(const char* name)
{
	if (name[0] != '\0') 
	{
		// Use tags for better network performance
		// Should be a tag, but we'll convert to one if it isn't.
		if (name[0] == StringTagPrefixByte)
			mSkinNameHandle = NetStringHandle(U32(dAtoi(name + 1)));
		else
			mSkinNameHandle = NetStringHandle(name);
	}
	else
		mSkinNameHandle = NetStringHandle();

	// Go ahead and call to reskin it.
	reSkin();
}

// Change the skins by referencing the mSkinNameHandle string.
void GuiObjectView::reSkin()
{
	if (mSkinNameHandle.isValidString())
	{
		Vector<String> skins;
		String(mSkinNameHandle.getString()).split(";", skins);

		for (S32 i = 0; i < skins.size(); i++)
		{
			String oldSkin(mAppliedSkinName.c_str());
			String newSkin(skins[i]);

			// Check if the skin handle contains an explicit "old" base string. This
			// allows all models to support skinning, even if they don't follow the 
			// "base_xxx" material naming convention.
			S32 split = newSkin.find('=');    // "old=new" format skin?
			if (split != String::NPos)
			{
				oldSkin = newSkin.substr(0, split);
				newSkin = newSkin.erase(0, split + 1);
			}

			TSShapeInstance *model = getModel();
			model->reSkin(newSkin, oldSkin);
			mAppliedSkinName = newSkin;
		}
	}
}
// <-- added( GuiObjectView sub-mesh Skinning ) --

//------------------------------------------------------------------------------

void GuiObjectView::setMountSkin( const String& name )
{
   if( mMountedModel )
      mMountedModel->reSkin( name, mMountSkinName );
      
   mMountSkinName = name;
}

//------------------------------------------------------------------------------

void GuiObjectView::setMountNode( S32 index )
{
   setMountNode( String::ToString( "mount%i", index ) );
}

//------------------------------------------------------------------------------

void GuiObjectView::setMountNode( const String& name )
{
   mMountNodeName = name;
   
   if( mModel )
      _initMount();
}

//------------------------------------------------------------------------------

void GuiObjectView::setMountedObject( const String& modelName )
{
   SAFE_DELETE( mMountedModel );
   mMountedModelName = String::EmptyString;

   // Load the model.
   
   Resource< TSShape > model = ResourceManager::get().load( modelName );
   if( !model )
   {
      Con::warnf( "GuiObjectView::setMountedObject -  Failed to load object model '%s'",
         modelName.c_str() );
      return;
   }

   mMountedModel = new TSShapeInstance( model, true );
   mMountedModelName = modelName;
   
   if( !mMountSkinName.isEmpty() )
      mMountedModel->reSkin( mMountSkinName );
   
   if( mModel )
      _initMount();
}

//------------------------------------------------------------------------------

// -- added( GuiObjectView extended Camera ) -->
void GuiObjectView::setUseNodes(bool use) 
{
	mUseNodes = use;
}

DefineEngineMethod(GuiObjectView, setUseNodes, void, (bool use), ,
	"Allow the GuiObjectView to use nodes for camera placement.\n"
	"Uses the shape's start01 node to set the camera position.\n")
{
	object->setUseNodes(use);
}

// <-- added( GuiObjectView extended Camera ) --

// -- added ( GuiObjectView sub-mesh Skinning ) -->
// Get the number of materials in the shape.
S32 GuiObjectView::getTargetCount()
{
	// Get the count.
	TSShapeInstance *model = getModel();
	S32 count = model->getTargetCount();
	return count;
}

// Get the name of the indexed material.
const char* GuiObjectView::getTargetName(S32 index)
{
	TSShapeInstance *model = getModel();
	const char* matName = model->getTargetName(index);
	return matName;
}

DefineEngineMethod(GuiObjectView, getTargetCount, S32, (), ,
	"Get the number of materials in the shape.\n"
	"@return the number of materials in the shape.\n"
	"@see getTargetName()\n")
{
	return object->getTargetCount();
}

DefineEngineMethod(GuiObjectView, getTargetName, const char*, (S32 index), (0),
	"Get the name of the indexed material.\n"
	"@param index index of the material to get (valid range is 0 - getTargetCount()-1).\n"
	"@return the name of the indexed material.\n"
	"@see getTargetCount()\n")
{
	return object->getTargetName(index);
}
// <-- added( GuiObjectView sub-mesh Skinning ) --

bool GuiObjectView::processCameraQuery( CameraQuery* query )
{
   // Adjust the camera so that we are still facing the model.
   
   Point3F vec;
   MatrixF xRot, zRot;
   xRot.set( EulerF( mCameraRot.x, 0.0f, 0.0f ) );
   zRot.set( EulerF( 0.0f, 0.0f, mCameraRot.z ) );

   mCameraMatrix.mul( zRot, xRot );
   mCameraMatrix.getColumn( 1, &vec );
   vec *= mOrbitDist;
   mCameraPos = mOrbitPos - vec;

   query->farPlane = 2100.0f;
   query->nearPlane = query->farPlane / 5000.0f;
   query->fov = 45.0f;
   mCameraMatrix.setColumn( 3, mCameraPos );
   query->cameraMatrix = mCameraMatrix;

   return true;
}

//------------------------------------------------------------------------------

void GuiObjectView::onMouseEnter( const GuiEvent & event )
{
   onMouseEnter_callback();
}

//------------------------------------------------------------------------------

void GuiObjectView::onMouseLeave( const GuiEvent & event )
{
   onMouseLeave_callback();
}

//------------------------------------------------------------------------------

void GuiObjectView::renderWorld( const RectI& updateRect )
{
   if( !mModel )
      return;
      
   GFXTransformSaver _saveTransforms;

   // Determine the camera position, and store off render state.
   RenderPassManager* renderPass = gClientSceneGraph->getDefaultRenderPass();

   S32 time = Platform::getVirtualMilliseconds();
   S32 dt = time - mLastRenderTime;
   mLastRenderTime = time;

   LIGHTMGR->unregisterAllLights();
   LIGHTMGR->setSpecialLight( LightManager::slSunLightType, mLight );
 
   GFX->setStateBlock( mDefaultGuiSB );

   FogData savedFogData = gClientSceneGraph->getFogData(); 
   gClientSceneGraph->setFogData(FogData());   

   SceneRenderState state
   (
      gClientSceneGraph,
      SPT_Diffuse,
	  SceneCameraState(GFX->getViewport(), mSaveFrustum, MatrixF::Identity, GFX->getProjectionMatrix()), // <-- updated: Use proper frustum.
      renderPass,
      true
   );

   renderPass->assignSharedXform(RenderPassManager::View, MatrixF::Identity); // <-- added ( GuiObjectView Sub-mesh Skinning ) -- 
   renderPass->assignSharedXform(RenderPassManager::Projection, GFX->getProjectionMatrix()); // <-- added ( GuiObjectView Sub-mesh Skinning ) --

   // Set up our TS render state here.   
   TSRenderState rdata;
   rdata.setSceneState( &state );

   // We might have some forward lit materials
   // so pass down a query to gather lights.
   LightQuery query;
   query.init( SphereF( Point3F::Zero, 1.0f ) );
   rdata.setLightQuery( &query );

   // Render primary model.
   if( mModel )
   {
      if( mRunThread )
      {
         mModel->advanceTime( dt / 1000.0f, mRunThread );

         mModel->animate();
      }
      
      mModel->render( rdata );
   }
   
   // Render mounted model.
   if( mMountedModel && mMountNode != -1 )
   {
      GFX->pushWorldMatrix();
      GFX->multWorld( mModel->mNodeTransforms[ mMountNode ] );
      GFX->multWorld( mMountTransform );
      
      mMountedModel->render( rdata );

      GFX->popWorldMatrix();
   }

   renderPass->renderPass( &state );
   
   gClientSceneGraph->setFogData(savedFogData);

   // Make sure to remove our fake sun.
   LIGHTMGR->unregisterAllLights();
}

//------------------------------------------------------------------------------

void GuiObjectView::setOrbitDistance( F32 distance )
{
   // Make sure the orbit distance is within the acceptable range
   mOrbitDist = mClampF( distance, mMinOrbitDist, mMaxOrbitDist );
}

//------------------------------------------------------------------------------

void GuiObjectView::setCameraSpeed( F32 factor )
{
   mCameraSpeed = factor;
}

//------------------------------------------------------------------------------

void GuiObjectView::setCameraRotation( const EulerF& rotation )
{
    mCameraRot.set(rotation);
}

//------------------------------------------------------------------------------
void GuiObjectView::setLightColor( const ColorF& color )
{
   mLightColor = color;
   if( mLight )
      mLight->setColor( color );
}

//------------------------------------------------------------------------------

void GuiObjectView::setLightAmbient( const ColorF& color )
{
   mLightAmbient = color;
   if( mLight )
      mLight->setAmbient( color );
}

//------------------------------------------------------------------------------

void GuiObjectView::setLightDirection( const Point3F& direction )
{
   mLightDirection = direction;
   if( mLight )
      mLight->setDirection( direction );
}

//------------------------------------------------------------------------------

void GuiObjectView::_initAnimation()
{
   AssertFatal( mModel, "GuiObjectView::_initAnimation - No model loaded!" );
   
   if( mAnimationSeqName.isEmpty() && mAnimationSeq == -1 )
      return;
      
   // Look up sequence by name.
            
   if( !mAnimationSeqName.isEmpty() )
   {
      mAnimationSeq = mModel->getShape()->findSequence( mAnimationSeqName );
      
      if( mAnimationSeq == -1 )
      {
         Con::errorf( "GuiObjectView::_initAnimation - Cannot find animation sequence '%s' on '%s'",
            mAnimationSeqName.c_str(),
            mModelName.c_str()
         );
         
         return;
      }
   }
   
   // Start sequence.
      
   if( mAnimationSeq != -1 )
   {
      if( mAnimationSeq >= mModel->getShape()->sequences.size() )
      {
         Con::errorf( "GuiObjectView::_initAnimation - Sequence '%i' out of range for model '%s'",
            mAnimationSeq,
            mModelName.c_str()
         );
         
         mAnimationSeq = -1;
         return;
      }
      
      if( !mRunThread )
         mRunThread = mModel->addThread();
         
      mModel->setSequence( mRunThread, mAnimationSeq, 0.f );
   }

   mLastRenderTime = Platform::getVirtualMilliseconds();
}


//------------------------------------------------------------------------------

void GuiObjectView::_initMount()
{
   AssertFatal( mModel, "GuiObjectView::_initMount - No model loaded!" );
      
   if( !mMountedModel )
      return;
      
   mMountTransform.identity();

   // Look up the node to which to mount to.
   
   if( !mMountNodeName.isEmpty() )
   {
      mMountNode = mModel->getShape()->findNode( mMountNodeName );
      if( mMountNode == -1 )
      {
         Con::errorf( "GuiObjectView::_initMount - No node '%s' on '%s'",
            mMountNodeName.c_str(),
            mModelName.c_str()
         );
         
         return;
      }
   }
   
   // Make sure mount node is valid.
   
   if( mMountNode != -1 && mMountNode >= mModel->getShape()->nodes.size() )
   {
      Con::errorf( "GuiObjectView::_initMount - Mount node index '%i' out of range for '%s'",
         mMountNode,
         mModelName.c_str()
      );
      
      mMountNode = -1;
      return;
   }
   
   // Look up node on the mounted model from
   // which to mount to the primary model's node.

   S32 mountPoint = mMountedModel->getShape()->findNode( "mountPoint" );
   if( mountPoint != -1 )
   {
      mMountedModel->getShape()->getNodeWorldTransform( mountPoint, &mMountTransform ),
      mMountTransform.inverse();
   }
}

//=============================================================================
//    Console Methods.
//=============================================================================
// MARK: ---- Console Methods ----

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, getModel, const char*, (),,
   "@brief Return the model displayed in this view.\n\n"
   "@tsexample\n"
   "// Request the displayed model name from the GuiObjectView object.\n"
   "%modelName = %thisGuiObjectView.getModel();\n"
   "@endtsexample\n\n"
   "@return Name of the displayed model.\n\n"
   "@see GuiControl")
{
   return Con::getReturnBuffer( object->getModelName() );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setModel, void, (const char* shapeName),,
   "@brief Sets the model to be displayed in this control.\n\n"
   "@param shapeName Name of the model to display.\n"
   "@tsexample\n"
   "// Define the model we want to display\n"
   "%shapeName = \"gideon.dts\";\n\n"
   "// Tell the GuiObjectView object to display the defined model\n"
   "%thisGuiObjectView.setModel(%shapeName);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setObjectModel( shapeName );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, getMountedModel, const char*, (),,
   "@brief Return the name of the mounted model.\n\n"
   "@tsexample\n"
   "// Request the name of the mounted model from the GuiObjectView object\n"
   "%mountedModelName = %thisGuiObjectView.getMountedModel();\n"
   "@endtsexample\n\n"
   "@return Name of the mounted model.\n\n"
   "@see GuiControl")
{
   return Con::getReturnBuffer( object->getMountedModelName() );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setMountedModel, void, (const char* shapeName),,
   "@brief Sets the model to be mounted on the primary model.\n\n"
   "@param shapeName Name of the model to mount.\n"
   "@tsexample\n"
   "// Define the model name to mount\n"
   "%modelToMount = \"GideonGlasses.dts\";\n\n"
   "// Inform the GuiObjectView object to mount the defined model to the existing model in the control\n"
   "%thisGuiObjectView.setMountedModel(%modelToMount);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setObjectModel(shapeName);
}

// -- added ( GuiObjectView sub-mesh Skinning ) --> 
DefineEngineMethod(GuiObjectView, getSkinName, const char*, (), ,
	"@brief Get the name of the skin applied to this shape.\n\n"
	"@return the name of the skin\n\n"

	"@see skin\n"
	"@see setSkinName()\n")
{

	return object->getSkinName();
}

DefineEngineMethod(GuiObjectView, setSkinName, void, (const char* skinName), ,
	"@brief Sets the skin to use on the model being displayed.\n\n"
	"@param skinName Name of the skin to use.\n"
	"@tsexample\n"
	"// Define the skin we want to apply to the main model in the control\n"
	"%skinName = \"disco_gideon\";\n\n"
	"// Inform the GuiObjectView control to update the skin the to defined skin\n"
	"%thisGuiObjectView.setSkin(%skinName);\n"
	"@endtsexample\n\n"
	"@see GuiControl")
{
	object->setSkinName(skinName);
}
// <-- added ( GuiObjectView sub-mesh Skinning ) --

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, getMountSkin, const char*, ( S32 param1, S32 param2),,
   "@brief Return the name of skin used on the mounted model.\n\n"
   "@tsexample\n"
   "// Request the skin name from the model mounted on to the main model in the control\n"
   "%mountModelSkin = %thisGuiObjectView.getMountSkin();\n"
   "@endtsexample\n\n"
   "@return Name of the skin used on the mounted model.\n\n"
   "@see GuiControl")
{
   return Con::getReturnBuffer( object->getMountSkin() );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setMountSkin, void, (const char* skinName),,
   "@brief Sets the skin to use on the mounted model.\n\n"
   "@param skinName Name of the skin to set on the model mounted to the main model in the control\n"
   "@tsexample\n"
   "// Define the name of the skin\n"
   "%skinName = \"BronzeGlasses\";\n\n"
   "// Inform the GuiObjectView Control of the skin to use on the mounted model\n"
   "%thisGuiObjectViewCtrl.setMountSkin(%skinName);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setMountSkin(skinName);
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setSeq, void, (const char* indexOrName),,
   "@brief Sets the animation to play for the viewed object.\n\n"
   "@param indexOrName The index or name of the animation to play.\n"
   "@tsexample\n"
   "// Set the animation index value, or animation sequence name.\n"
   "%indexVal = \"3\";\n"
   "//OR:\n"
   "%indexVal = \"idle\";\n\n"
   "// Inform the GuiObjectView object to set the animation sequence of the object in the control.\n"
   "%thisGuiObjectVew.setSeq(%indexVal);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   if( dIsdigit( indexOrName[0] ) )
      object->setObjectAnimation( dAtoi( indexOrName ) );
   else
      object->setObjectAnimation( indexOrName );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setMount, void, ( const char* shapeName, const char* mountNodeIndexOrName),,
   "@brief Mounts the given model to the specified mount point of the primary model displayed in this control.\n\n"
   "Detailed description\n\n"
   "@param shapeName Name of the model to mount.\n"
   "@param mountNodeIndexOrName Index or name of the mount point to be mounted to. If index, corresponds to \"mountN\" in your shape where N is the number passed here.\n"
   "@tsexample\n"
   "// Set the shapeName to mount\n"
   "%shapeName = \"GideonGlasses.dts\"\n\n"
   "// Set the mount node of the primary model in the control to mount the new shape at\n"
   "%mountNodeIndexOrName = \"3\";\n"
   "//OR:\n"
   "%mountNodeIndexOrName = \"Face\";\n\n"
   "// Inform the GuiObjectView object to mount the shape at the specified node.\n"
   "%thisGuiObjectView.setMount(%shapeName,%mountNodeIndexOrName);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   if( dIsdigit( mountNodeIndexOrName[0] ) )
      object->setMountNode( dAtoi( mountNodeIndexOrName ) );
   else
      object->setMountNode( mountNodeIndexOrName );
      
   object->setMountedObject( shapeName );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, getOrbitDistance, F32, (),,
   "@brief Return the current distance at which the camera orbits the object.\n\n"
   "@tsexample\n"
   "// Request the current orbit distance\n"
   "%orbitDistance = %thisGuiObjectView.getOrbitDistance();\n"
   "@endtsexample\n\n"
   "@return The distance at which the camera orbits the object.\n\n"
   "@see GuiControl")
{
   return object->getOrbitDistance();
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setOrbitDistance, void, (F32 distance),,
   "@brief Sets the distance at which the camera orbits the object. Clamped to the acceptable range defined in the class by min and max orbit distances.\n\n"
   "Detailed description\n\n"
   "@param distance The distance to set the orbit to (will be clamped).\n"
   "@tsexample\n"
   "// Define the orbit distance value\n"
   "%orbitDistance = \"1.5\";\n\n"
   "// Inform the GuiObjectView object to set the orbit distance to the defined value\n"
   "%thisGuiObjectView.setOrbitDistance(%orbitDistance);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setOrbitDistance( distance );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, getCameraSpeed, F32, (),,
   "@brief Return the current multiplier for camera zooming and rotation.\n\n"
   "@tsexample\n"
   "// Request the current camera zooming and rotation multiplier value\n"
   "%multiplier = %thisGuiObjectView.getCameraSpeed();\n"
   "@endtsexample\n\n"
   "@return Camera zooming / rotation multiplier value.\n\n"
   "@see GuiControl")
{
   return object->getCameraSpeed();
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setCameraSpeed, void, (F32 factor),,
   "@brief Sets the multiplier for the camera rotation and zoom speed.\n\n"
   "@param factor Multiplier for camera rotation and zoom speed.\n"
   "@tsexample\n"
   "// Set the factor value\n"
   "%factor = \"0.75\";\n\n"
   "// Inform the GuiObjectView object to set the camera speed.\n"
   "%thisGuiObjectView.setCameraSpeed(%factor);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setCameraSpeed( factor );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setLightColor, void, ( ColorF color),,
   "@brief Set the light color on the sun object used to render the model.\n\n"
   "@param color Color of sunlight.\n"
   "@tsexample\n"
   "// Set the color value for the sun\n"
   "%color = \"1.0 0.4 0.5\";\n\n"
   "// Inform the GuiObjectView object to change the sun color to the defined value\n"
   "%thisGuiObjectView.setLightColor(%color);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{  
   object->setLightColor( color );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setLightAmbient, void, (ColorF color),,
   "@brief Set the light ambient color on the sun object used to render the model.\n\n"
   "@param color Ambient color of sunlight.\n"
   "@tsexample\n"
   "// Define the sun ambient color value\n"
   "%color = \"1.0 0.4 0.6\";\n\n"
   "// Inform the GuiObjectView object to set the sun ambient color to the requested value\n"
   "%thisGuiObjectView.setLightAmbient(%color);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setLightAmbient( color );
}

//-----------------------------------------------------------------------------

DefineEngineMethod( GuiObjectView, setLightDirection, void, (Point3F direction),,
   "@brief Set the light direction from which to light the model.\n\n"
   "@param direction XYZ direction from which the light will shine on the model\n"
   "@tsexample\n"
   "// Set the light direction\n"
   "%direction = \"1.0 0.2 0.4\"\n\n"
   "// Inform the GuiObjectView object to change the light direction to the defined value\n"
   "%thisGuiObjectView.setLightDirection(%direction);\n"
   "@endtsexample\n\n"
   "@see GuiControl")
{
   object->setLightDirection( direction );
}
