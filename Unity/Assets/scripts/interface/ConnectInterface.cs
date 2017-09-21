using System;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

public class ConnectInterface:MonoBehaviour
{
	[HideInInspector]
	public MainInterface mainInterface;
    //time it takes from the last notification for the indicator to fade away completely
    public double fadeTime = 1f;
	
    /// <summary>
    /// Add a new connection panel to the connection interface.
    /// </summary>
    /// <param name="ID">The ID of the glove this connection panel is linked with.</param>
    /// <param name="UUID">The UUID of this panel's corresponding glove.</param>
	public void AddPanel(int ID, string UUID)
	{
		//instantiate a new panel from the prefab
		GameObject panel = Instantiate((GameObject)Resources.Load("prefabs/Glove Panel", typeof(GameObject)), transform);
		//position the panel in the list (stack from bottom to top)
		panel.transform.localPosition += Vector3.up * (35f * (transform.childCount-1)+5f);
		panel.name = panel.transform.GetChild(0).GetComponent<Text>().text = "Glove " + ID.ToString();
		
		//set up event system
		EventTrigger trigger = (EventTrigger)panel.AddComponent(typeof(EventTrigger));
		//when hovering mouse over panel, highlight corresponding glove
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[0].eventID = EventTriggerType.PointerEnter;
		trigger.triggers[0].callback.AddListener(state => EnterPanel(ID));
		//when exiting panel with mouse, remove highlight
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[1].eventID = EventTriggerType.PointerExit;
		trigger.triggers[1].callback.AddListener(state => ExitPanel());
		panel.transform.GetChild(1).GetComponent<Text>().text = UUID;
        //set up buttons
        Button addButton = panel.transform.GetChild(2).GetComponent<Button>();
        addButton.onClick.AddListener(() => AddGlove(ID, addButton));
        Button connectButton = panel.transform.GetChild(3).GetComponent<Button>();
        connectButton.onClick.AddListener(() => ConnectGlove(ID, connectButton));
        Button clearButton = panel.transform.GetChild(4).GetComponent<Button>();
        clearButton.onClick.AddListener(() => mainInterface.ClearGlove(ID));
	}

    /// <summary>
    /// Add a new glove to the world, or remove a glove from the world.
    /// </summary>
    /// <param name="ID">The glove to add or remove.</param>
    /// <param name="thisButton">The button that is being pressed.</param>
    private void AddGlove(int ID, Button thisButton)
    {
        //treat the button as a toggle based on its current text
        Text thisText = thisButton.transform.GetChild(0).GetComponent<Text>();
        if (thisText.text == "Add")
        {
            thisText.text = "Remove";
            mainInterface.AddGlove(ID);
            //enable the connect button
            thisButton.transform.parent.GetChild(3).gameObject.GetComponent<Button>().interactable = true;
        }
        else
        {
            thisText.text = "Add";
            mainInterface.RemoveGlove(ID);
            //disable other buttons and disconnect the glove if it's connected
            thisButton.transform.parent.GetChild(3).GetChild(0).gameObject.GetComponent<Text>().text = "Connect";
            thisButton.transform.parent.GetChild(3).gameObject.GetComponent<Button>().interactable = false;
            thisButton.transform.parent.GetChild(4).gameObject.GetComponent<Button>().interactable = false;
        }
    }

    /// <summary>
    /// Connect or disconnect an existing glove.
    /// </summary>
    /// <param name="ID">The glove to connect or disconnect.</param>
    /// <param name="thisButton">The button that is being pressed.</param>
    private void ConnectGlove(int ID, Button thisButton)
    {
        Text thisText = thisButton.transform.GetChild(0).GetComponent<Text>();
        if (thisText.text == "Connect" && mainInterface.ConnectGlove(ID))
        {
            thisText.text = "Disconnect";
            transform.GetChild(ID).GetChild(5).gameObject.SetActive(true);
            thisButton.transform.parent.GetChild(4).gameObject.GetComponent<Button>().interactable = true;
        }
        else if (thisText.text == "Disconnect" && mainInterface.DisconnectGlove(ID))
        {
            thisText.text = "Connect";
            transform.GetChild(ID).GetChild(5).gameObject.SetActive(false);
            thisButton.transform.parent.GetChild(4).gameObject.GetComponent<Button>().interactable = false;
        }
    }

    /// <summary>
    /// Update the panel's notification indicator.
    /// </summary>
    /// <param name="ID">The panel to update.</param>
    /// <param name="lastNotification">The time since the last notification was received for this glove.</param>
    public void UpdateNotificationIndicator(int ID, double lastNotification)
    {
        transform.GetChild(ID).GetChild(5).gameObject.GetComponent<Image>().color = Color.Lerp(new Color(0.0f, 0.68f, 0.934f), new Color(0.0f, 0.68f, 0.934f, 0f), (float)(lastNotification / fadeTime));
    }
	
    /// <summary>
    /// Highlight a panel's corresponding glove.
    /// </summary>
    /// <param name="ID">The glove to highlight.</param>
	private void EnterPanel(int ID)
	{
		//if this panel's glove has been added, highlight it in the 3D view
		if (mainInterface.gloves.ContainsKey(ID)) Camera.main.GetComponent<HighlightEffect>().highlightObject = mainInterface.gloves[ID].gameObject;
	}
	
    /// <summary>
    /// Remove the highlight from the glove.
    /// </summary>
	private void ExitPanel()
	{
		//remove the highlight effect from the glove
		Camera.main.GetComponent<HighlightEffect>().highlightObject = null;
	}
}
