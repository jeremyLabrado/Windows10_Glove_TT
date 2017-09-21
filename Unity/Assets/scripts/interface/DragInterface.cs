using UnityEngine;
using UnityEngine.EventSystems;

public class DragInterface:MonoBehaviour
{
	[HideInInspector]
	public MainInterface mainInterface;
	//the position of the mouse in the last frame
	private Vector3 lastPos = Vector3.zero;
	//the target to center the camera's orbit around
	[HideInInspector]
	public Vector3 target = Vector3.zero;
	//the proportion of the camera's orbit to the mouse's drag
	public float sensitivity = 0.2f;

	void Start()
	{
		//set up event system for this panel
		EventTrigger trigger = (EventTrigger)gameObject.AddComponent(typeof(EventTrigger));
		//when mouse clicks, set lastPos for first drag event
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[0].eventID = EventTriggerType.PointerDown;
		trigger.triggers[0].callback.AddListener(state => lastPos = Input.mousePosition);
		//when dragging, rotate the camera
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[1].eventID = EventTriggerType.Drag;
		trigger.triggers[1].callback.AddListener(state => DragCamera());
	}
	
	void DragCamera()
	{
		//calculate how much the mouse has moved since the last frame
		Vector3 drag = (Input.mousePosition - lastPos)*sensitivity;
		Transform cam = Camera.main.transform;
        //rotate around orbit center based on drag coordinates
        float yAngle = (cam.localPosition-target).y / (cam.localPosition - target).magnitude;
        cam.RotateAround(target, Vector3.up, drag.x);
        //don't allow vertical rotation to hit the top or bottom, to avoid locking
        if (Mathf.Sign(yAngle) *drag.y > 0 || Mathf.Abs(yAngle) < 0.9f) cam.RotateAround(target, cam.right, -drag.y);
		//face toward the orbit center
		cam.LookAt(target);
		lastPos = Input.mousePosition;
	}
}
