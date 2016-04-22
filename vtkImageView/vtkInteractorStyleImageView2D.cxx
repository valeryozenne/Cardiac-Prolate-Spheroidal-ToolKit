#include "vtkInteractorStyleImageView2D.h"
#include "vtkCallbackCommand.h"

#include "vtkAbstractPropPicker.h"
#include "vtkAssemblyPath.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkRenderWindowInteractor.h"
#include <vtkImageData.h>
#include <vtkRenderWindow.h>
#include <vtkCamera.h>
#include <vtkRenderer.h>
#include "vtkCommand.h"

#include <vtkImageView2DCommand.h>



vtkStandardNewMacro (vtkInteractorStyleImageView2D);


vtkInteractorStyleImageView2D::vtkInteractorStyleImageView2D()
  : vtkInteractorStyleImage()
{

  this->SliceStep = 0;
  this->RequestedPosition[0] = this->RequestedPosition[1] = 0;

  this->LeftButtonInteraction   = InteractionTypeWindowLevel;
  this->RightButtonInteraction  = InteractionTypeZoom;
  this->MiddleButtonInteraction = InteractionTypePan;
  this->WheelButtonInteraction  = InteractionTypeSlice;
}


vtkInteractorStyleImageView2D::~vtkInteractorStyleImageView2D() 
{
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnMouseMove() 
{
  switch (this->State) 
  {
      case VTKIS_SLICE_MOVE:
	this->SliceMove();
	this->InvokeEvent(vtkImageView2DCommand::SliceMoveEvent, this);
	break;
      case VTKIS_DOLLY:
      case VTKIS_ZOOM:
 	this->Superclass::OnMouseMove();
	this->InvokeEvent(vtkImageView2DCommand::CameraZoomEvent, this);
	this->InvokeEvent(vtkImageView2DCommand::CameraMoveEvent, this);
	break;
      case VTKIS_PAN:
	this->Superclass::OnMouseMove();
	this->InvokeEvent(vtkImageView2DCommand::CameraPanEvent, this);
	this->InvokeEvent(vtkImageView2DCommand::CameraMoveEvent, this);
	break;
      case VTKIS_SPIN:
      case VTKIS_ROTATE:
      case VTKIS_FORWARDFLY:
      case VTKIS_REVERSEFLY:
	this->Superclass::OnMouseMove();
	this->InvokeEvent(vtkImageView2DCommand::CameraMoveEvent, this);
	break;
      case VTKIS_NONE:
	this->DefaultMoveAction();
	break;
      default:
	this->Superclass::OnMouseMove();
	break;
  }
}


//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnLeftButtonDown() 
{

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];  
  this->FindPokedRenderer(x, y);

  if (this->Interactor->GetRepeatCount())
  {
    this->RequestedPosition[0] = x;
    this->RequestedPosition[1] = y;
    this->InvokeEvent (vtkImageView2DCommand::RequestedPositionEvent, this);
    return;
  }  
  
  this->GrabFocus(this->EventCallbackCommand);
	
  if (this->Interactor->GetShiftKey() || this->Interactor->GetControlKey()) 
  {
    if (this->GetLeftButtonInteraction() == InteractionTypeWindowLevel)
      this->StartSliceMove();
  }
  else
  {
    switch(this->GetLeftButtonInteraction())
    {
	case InteractionTypeSlice:
	  this->RequestedPosition[0] = x;
	  this->RequestedPosition[1] = y;
	  this->InvokeEvent (vtkImageView2DCommand::RequestedPositionEvent, this);
	  this->StartSliceMove();
	  break;
	case InteractionTypeWindowLevel:
	  this->Superclass::OnLeftButtonDown();
	  break;
	case InteractionTypeZoom:
	  this->Superclass::OnRightButtonDown();
	  break;
	case InteractionTypePan:
	  this->Superclass::OnMiddleButtonDown();
	  break;
	default:
	  break;
    }
  }
}


//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnLeftButtonUp()
{
  switch (this->State) 
  {
      case VTKIS_SLICE_MOVE:
        this->EndSliceMove();
	if ( this->Interactor )
	{
	  this->ReleaseFocus();
	}
	break;
		  
      default:
	break;
  }

  switch (this->LeftButtonInteraction)
  {
      case InteractionTypeSlice :
	break;
      case InteractionTypeZoom :
	this->Superclass::OnRightButtonUp();
	break;
      case InteractionTypePan :
	this->Superclass::OnMiddleButtonUp();
	break;
      case InteractionTypeWindowLevel :
      default:
	this->Superclass::OnLeftButtonUp();
	break;
  }  
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnMiddleButtonDown()
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if ( !this->CurrentRenderer )
    return;


  this->GrabFocus(this->EventCallbackCommand);
	
  switch(this->GetMiddleButtonInteraction())
  {
      case InteractionTypeSlice:
	this->StartSliceMove();
	break;
      case InteractionTypeWindowLevel:
	this->Superclass::OnLeftButtonDown();
	break;
      case InteractionTypeZoom:
	this->Superclass::OnRightButtonDown();
	break;
      case InteractionTypePan:
	this->Superclass::OnMiddleButtonDown();
	break;
      default:
	break;
  }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnMiddleButtonUp()
{
  switch (this->State)
  {
      case VTKIS_SLICE_MOVE:
        this->EndSliceMove();
	if ( this->Interactor )
	  this->ReleaseFocus();
        break;
      default:
	break;
  }

  switch (this->MiddleButtonInteraction)
  {
      case InteractionTypeSlice :
	break;
      case InteractionTypeZoom :
	this->Superclass::OnRightButtonUp();
	break;
      case InteractionTypeWindowLevel :
	this->Superclass::OnLeftButtonUp();
	break;
      case InteractionTypePan :
      default:
	this->Superclass::OnMiddleButtonUp();
	break;
  }
  
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnRightButtonDown() 
{
  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];
  
  this->FindPokedRenderer(x, y);
  if ( !this->CurrentRenderer )
    return;
  
  this->GrabFocus(this->EventCallbackCommand);
  
  switch(this->GetRightButtonInteraction())
  {
      case InteractionTypeSlice:
	this->StartSliceMove();
	break;
      case InteractionTypeWindowLevel:
	this->Superclass::OnLeftButtonDown();
	break;
      case InteractionTypeZoom:
	this->Superclass::OnRightButtonDown();
	break;
	
      case InteractionTypePan:
	this->Superclass::OnMiddleButtonDown();
	break;
      default:
	break;
  }
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnRightButtonUp() 
{
  switch (this->State) 
  {
      case VTKIS_SLICE_MOVE:
        this->EndSliceMove();
	if ( this->Interactor )
	{
	  this->ReleaseFocus();
	}
        break;
      default:
	break;
  }

  switch (this->RightButtonInteraction)
  {
      case InteractionTypeSlice :
	break;
      case InteractionTypePan :
	this->Superclass::OnMiddleButtonUp();
	break;
      case InteractionTypeWindowLevel :
	this->Superclass::OnLeftButtonUp();
	break;
      case InteractionTypeZoom :
      default:
	this->Superclass::OnRightButtonUp();
	break;
  }
}
//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnMouseWheelForward() 
{

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if ( !this->CurrentRenderer )
    return;

  switch(this->GetWheelButtonInteraction())
  {
      case InteractionTypeSlice:
	this->SliceStep = 1;
	this->InvokeEvent (vtkImageView2DCommand::SliceMoveEvent, this);
	break;
      case InteractionTypeWindowLevel:
	this->Superclass::OnMouseWheelForward();
	break;
      case InteractionTypeZoom:
	this->Superclass::OnMouseWheelForward();
	break;
      case InteractionTypePan:
	this->Superclass::OnMouseWheelForward();
	break;
      default:
	break;
  }

}
//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnMouseWheelBackward() 
{

  int x = this->Interactor->GetEventPosition()[0];
  int y = this->Interactor->GetEventPosition()[1];

  this->FindPokedRenderer(x, y);
  if ( !this->CurrentRenderer )
    return;

  switch(this->GetWheelButtonInteraction())
  {
      case InteractionTypeSlice:
	this->SliceStep = -1;
	this->InvokeEvent (vtkImageView2DCommand::SliceMoveEvent, this);
	break;
      case InteractionTypeWindowLevel:
	this->Superclass::OnMouseWheelBackward();
	break;
      case InteractionTypeZoom:
	this->Superclass::OnMouseWheelBackward();
	break;
      case InteractionTypePan:
	this->Superclass::OnMouseWheelBackward();
	break;
      default:
	break;
  }  
}



//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnChar() 
{
  vtkRenderWindowInteractor *rwi = this->Interactor;

  if (strcmp (rwi->GetKeySym(),"Up")==0 || strcmp (rwi->GetKeySym(),"Down")==0)
  {
    if (strcmp (rwi->GetKeySym(),"Up")==0)
      this->SliceStep = 1;
    else
      this->SliceStep = -1;

    switch (this->KeyboardInteraction) {
	case InteractionTypeSlice:
	  this->InvokeEvent (vtkImageView2DCommand::SliceMoveEvent, this);
	  break;
	default:
	  break;
	  /*
	    case InteractionTypeWindowLevel:
	    //this->InvokeEvent (vtkImageView2DCommand::InteractionTypeWindowLevel, this);
	    break;
	    case InteractionTypeZoom:
	    this->InvokeEvent (vtkImageView2DCommand::CameraZoomEvent, this);
	    break;
	    case InteractionTypePan:
	    this->InvokeEvent (vtkImageView2DCommand::CameraPanEvent, this);
	    break;
	  */
    }

  }
  else if ((rwi->GetKeyCode() == 'r') || (rwi->GetKeyCode() == 'R'))
  {
    this->InvokeEvent (vtkImageView2DCommand::ResetViewerEvent, this);
  }
  this->Superclass::OnChar();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnKeyDown(void)
{
  // Apparently there is a problem here.
  // The event vtkCommand::CharEvent and vtkCommand::KeyPressEvent seem
  // to mix between each other.
  // tackled by calling the charevent
  // (supposed to be called at any keyboard event)
  this->OnChar();
  this->Superclass::OnKeyDown();
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::OnKeyUp(void)
{
  this->Superclass::OnKeyUp();
}


//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::StartSliceMove()
{
  if ((this->State != VTKIS_NONE) && (this->State != VTKIS_PICK)) {
    return;
  }
  this->StartState(VTKIS_SLICE_MOVE);
  this->InvokeEvent(vtkImageView2DCommand::StartSliceMoveEvent, this);
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::EndSliceMove()
{
  if (this->State != VTKIS_SLICE_MOVE) {
    return;
  }
  this->StopState();
  this->InvokeEvent(vtkImageView2DCommand::EndSliceMoveEvent, this);
  
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::SliceMove()
{
  vtkRenderWindowInteractor *rwi = this->Interactor;
  int dy = rwi->GetEventPosition()[1] - rwi->GetLastEventPosition()[1];
  this->SliceStep = dy;
}

//----------------------------------------------------------------------------
void vtkInteractorStyleImageView2D::DefaultMoveAction()
{
  this->InvokeEvent (vtkImageView2DCommand::DefaultMoveEvent ,this);
}
