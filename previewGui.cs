
///----------------------------------------------------------------------------
/// PreviewGui
///----------------------------------------------------------------------------
function PreviewGui::onWake(%this)
{
	%this.setUseNodes( true ); 
	%this.resetCam();
}

function PreviewGui::resetCam( %this ) 
{
	%dist = %this.getOrbitDistance();

	echo( "PreviewGui::resetCam() - %dist : " @ %dist @ "" );

	if( !isEventPending( PreviewGui.moveSchedule ) ) 
		PreviewGui.moveSchedule = PreviewGui.schedule( 1, "moveCamHome", %dist );
	else {
		cancel( PreviewGui.moveSchedule );
		PreviewGui.moveSchedule = PreviewGui.schedule( 1, "moveCamHome", %dist );	
	}		
}

function PreviewGui::moveCamHome( %this, %dist )
{
	%xyz = %this.getOrbitPos();
	%x = getWord( %xyz, 0 );
	%y = getWord( %xyz, 1 );
	%z = getWord( %xyz, 2 );
	
	%dist += 0.01; // Increment the distance.
	%z -= 0.01;

	if( %dist > %this.maxOrbitDistance )
		%dist = %this.maxOrbitDistance;
		
	if( %z < 1 )
		%z = 1;		
		
	%this.setOrbitDistance( %dist );
	%newXYZ = %x SPC %y SPC %z;
	%this.setOrbitPos( %newXYZ );
	
	if( %this.getOrbitDistance() < %this.maxOrbitDistance )
		PreviewGui.moveSchedule = PreviewGui.schedule( 10, "moveCamHome", %dist );
	else if( %this.getOrbitDistance() == %this.maxOrbitDistance && %z < 1 )
		PreviewGui.moveSchedule = PreviewGui.schedule( 10, "moveCamHome", %dist );	
}