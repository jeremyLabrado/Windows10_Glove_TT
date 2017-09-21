using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;

public class PairInterface:MonoBehaviour
{
	[HideInInspector]
	public MainInterface mainInterface;
	
	//positions for the center of each glove pair
	private List<Vector3> glovePositions = new List<Vector3>(){Vector3.zero, Vector3.up * 2f, Vector3.up * 4f};
	//how far apart the gloves of each pair should be
	private float gloveSeparation = 2.6f;
	
	//where the mousedrag started, to position the pairblock properly
	private Vector3 dragStart;
	//the slot the pairblock being dragged was removed from
	private Transform originalParent;
	
	//the left and right halves of the pairslots
	private List<GameObject> leftPairSlots = new List<GameObject>();
	private List<GameObject> rightPairSlots = new List<GameObject>();
	
    /// <summary>
    /// Add a new pairblock controlling the handedness and pairing of a corresponding glove object.
    /// </summary>
    /// <param name="ID">The ID of the corresponding glove.</param>
	public void AddPairBlock(int ID)
	{
		//find a slot in which this pairblock can be initialized
		GameObject pairSlot = FindSlot();
		
		//create a new pairblock
		GameObject pairBlock = new GameObject();
		//define the pairblock's size(18*18, leaving 1px on each side for the pairslot around it)
		RectTransform pairBlockRect = (RectTransform)pairBlock.AddComponent(typeof(RectTransform));
		pairBlockRect.sizeDelta = new Vector2(18f,18f);
		pairBlock.AddComponent(typeof(CanvasRenderer));
		pairBlock.AddComponent(typeof(Image));
		//slot the pairblock into the pairslot
		pairBlock.transform.SetParent(pairSlot.transform);
		pairBlockRect.anchoredPosition = Vector3.zero;
		pairBlock.name = ID.ToString();
		
		//define a new object for the text, as Image and Text cannot coexist
		GameObject pairText = new GameObject();
		RectTransform pairTextRect = (RectTransform)pairText.AddComponent(typeof(RectTransform));
		pairTextRect.sizeDelta = new Vector2(18f,18f);
		pairText.AddComponent(typeof(CanvasRenderer));
		Text pairTextText = (Text)pairText.AddComponent(typeof(Text));
		pairTextText.text = ID.ToString();
		pairTextText.alignment = TextAnchor.MiddleCenter;
		pairTextText.font = (Font)Resources.GetBuiltinResource(typeof(Font), "Arial.ttf");
		pairText.transform.SetParent(pairBlock.transform);
		pairText.transform.localPosition = Vector3.zero;
		pairText.name = ID.ToString() + " Text";
		
		//update glove handedness
		int pairHand = (pairSlot.name[0] == 'R') ? 1 : -1;
		//colorcode pairblock
		if (pairHand == 1) pairBlock.GetComponent<Image>().color = Color.red;
		else pairBlock.GetComponent<Image>().color = Color.blue;
		//update the corresponding glove object
		if (mainInterface.gloves.ContainsKey(ID)) 
		{
			mainInterface.gloves[ID].GetComponent<HandController>().handedness = pairHand;
			//set position
			int pair = (int)Char.GetNumericValue(pairSlot.name[1]);
			mainInterface.gloves[ID].transform.localPosition = glovePositions[pair] + pairHand * Vector3.right * gloveSeparation;
		}
		
		//set up event system
		EventTrigger trigger = (EventTrigger)pairBlock.AddComponent(typeof(EventTrigger));
		//on drag, make the pairblock follow the mouse
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[0].eventID = EventTriggerType.Drag;
		trigger.triggers[0].callback.AddListener(state => pairBlock.transform.position = Input.mousePosition - dragStart);
		//on click, record original pairslot and free the pairblock to be dragged
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[1].eventID = EventTriggerType.PointerDown;
		trigger.triggers[1].callback.AddListener(state => { originalParent = pairBlock.transform.parent; pairBlock.transform.SetParent(transform); dragStart = Input.mousePosition - pairBlock.transform.position; });
		//on mouse release, drop the pairblock, into a new pairslot or back to its original pairslot
		trigger.triggers.Add(new EventTrigger.Entry());
		trigger.triggers[2].eventID = EventTriggerType.PointerUp;
		trigger.triggers[2].callback.AddListener(state => DropPairBlock(pairBlock));
		
		//if all slots are filled, this will add a new empty pair
		FindSlot();
	}
	
    /// <summary>
    /// Remove a pairblock from the pairing interface.
    /// </summary>
    /// <param name="ID">The ID of the glove to remove the pairblock for.</param>
    public void RemovePairBlock(int ID)
    {
        string name = ID.ToString();
        for (int i = 0; i < leftPairSlots.Count; i++)
        {
            if (leftPairSlots[i].transform.childCount > 0 && leftPairSlots[i].transform.GetChild(0).gameObject.name == name)
            {
                GameObject.Destroy(leftPairSlots[i].transform.GetChild(0).gameObject);
                return;
            }
            if (rightPairSlots[i].transform.childCount > 0 && rightPairSlots[i].transform.GetChild(0).gameObject.name == name)
            {
                GameObject.Destroy(rightPairSlots[i].transform.GetChild(0).gameObject);
                return;
            }
        }
    }

    /// <summary>
    /// Find a pairslot to add a pairblock to. Generates new pairslots if necessary.
    /// </summary>
    /// <returns>An empty pairslot.</returns>
	private GameObject FindSlot()
	{
		//loop through all the pairslots
		for (int i = 0; i < leftPairSlots.Count; i++)
		{
			//if a pairslot has no children (i.e. is empty), return it
			if (leftPairSlots[i].transform.childCount == 0) return leftPairSlots[i];
			if (rightPairSlots[i].transform.childCount == 0) return rightPairSlots[i];
		}
		//if there are no empty pairslots, add a new pair
		return AddPair();
	}
	
    /// <summary>
    /// Adds a new pair of pairslots to the pairing interface.
    /// </summary>
    /// <returns>The left pairslot of the new pair.</returns>
	private GameObject AddPair()
	{
		int pair = leftPairSlots.Count;
		//make a new pairslot
		GameObject leftSlot = AddPairSlot();
		RectTransform leftRect = leftSlot.GetComponent<RectTransform>();
		leftSlot.name = "L" + pair.ToString();
		//place the pairslot's origin at the middle-top
		leftRect.anchorMin = new Vector2(0.5f, 1f);
		leftRect.anchorMax = new Vector2(0.5f, 1f);
		leftRect.pivot = new Vector2(0.5f, 1f);
		leftSlot.transform.localPosition += Vector3.left*(15f) + Vector3.down*(pair*25f+10f);
		leftPairSlots.Add(leftSlot);
		
		//reproduce the process for a matching pairslot
		GameObject rightSlot = AddPairSlot();
		RectTransform rightRect = rightSlot.GetComponent<RectTransform>();
		rightSlot.name = "R" + pair.ToString();
		rightSlot.GetComponent<RectTransform>();
		rightRect.anchorMin = new Vector2(0.5f, 1f);
		rightRect.anchorMax = new Vector2(0.5f, 1f);
		rightRect.pivot = new Vector2(0.5f, 1f);
		rightSlot.transform.localPosition += Vector3.right*(15f) + Vector3.down*(pair*25f+10f);
		rightPairSlots.Add(rightSlot);
		
		//return the left pairslot of this new pair
		return leftSlot;
	}
	
    /// <summary>
    /// Creates a pairslot.
    /// </summary>
    /// <returns>The new pairslot.</returns>
	private GameObject AddPairSlot()
	{
		//make a new pairslot
		GameObject pairSlot = new GameObject();
		pairSlot.transform.SetParent(transform, false);
		RectTransform pairSlotRect = (RectTransform)pairSlot.AddComponent(typeof(RectTransform));
		pairSlotRect.sizeDelta = new Vector2(20f,20f);
		pairSlot.AddComponent(typeof(CanvasRenderer));
		Image pairSlotImage = (Image)pairSlot.AddComponent(typeof(Image));
		pairSlotImage.color = Color.black;
		return pairSlot;
	}
	
    /// <summary>
    /// Drops a pairblock into a new pairslot.
    /// </summary>
    /// <param name="pairBlock">The pairblock that was just dropped.</param>
	private void DropPairBlock(GameObject pairBlock)
	{
		//reassigns pairblock to a pairslot the mouse is over
		//if pairblock is not dropped on pairslot, return to original pairslot
		Transform newParent = originalParent;
		//loop through open slots
		for (int i = 0; i < leftPairSlots.Count; i++)
		{
			//check mouse position against available pairslots
			Rect slotRect = leftPairSlots[i].GetComponent<RectTransform>().rect;
			if (Input.mousePosition.x >= leftPairSlots[i].transform.position.x - (slotRect.width/2f+2) &&
				Input.mousePosition.y >= leftPairSlots[i].transform.position.y - (slotRect.height/2f+2) &&
				Input.mousePosition.x <= leftPairSlots[i].transform.position.x + (slotRect.width/2f+2) &&
				Input.mousePosition.y <= leftPairSlots[i].transform.position.y + (slotRect.height/2f+2))
			{
				newParent = leftPairSlots[i].transform;
				break;
			}
			slotRect = leftPairSlots[i].GetComponent<RectTransform>().rect;
			if (Input.mousePosition.x >= rightPairSlots[i].transform.position.x - slotRect.width/2f &&
				Input.mousePosition.y >= rightPairSlots[i].transform.position.y - slotRect.height/2f &&
				Input.mousePosition.x <= rightPairSlots[i].transform.position.x + slotRect.width/2f &&
				Input.mousePosition.y <= rightPairSlots[i].transform.position.y + slotRect.height/2f)
			{
				newParent = rightPairSlots[i].transform;
				break;
			}
		}
		
		//swap out old pairblock if this pairslot was occupied
		if (newParent.childCount != 0)
		{
			GameObject swapBlock = newParent.GetChild(0).gameObject;
			//slot new pairblock into old pairslot
			swapBlock.transform.SetParent(originalParent);
			swapBlock.GetComponent<RectTransform>().anchoredPosition = Vector3.zero;
			//update handedness
			int swapHand = (swapBlock.transform.parent.name[0] == 'R') ? 1 : -1;
			if (swapHand == 1) swapBlock.GetComponent<Image>().color = Color.red;
			else swapBlock.GetComponent<Image>().color = Color.blue;
			swapBlock.GetComponent<RectTransform>().anchoredPosition = Vector3.zero;
			//find the ID from the pairblock's name
			int swapID = Int32.Parse(swapBlock.name);
			if (mainInterface.gloves.ContainsKey(swapID)) 
			{
				mainInterface.gloves[swapID].GetComponent<HandController>().handedness = swapHand;
				//flip the glove's texture
				mainInterface.gloves[swapID].GetComponent<HandController>().UpdateTexture();
				int pair = Int32.Parse(swapBlock.transform.parent.name.Substring(1));
				mainInterface.gloves[swapID].transform.localPosition = glovePositions[pair] + swapHand * Vector3.right * gloveSeparation;
			}
		}
		//slot pairblock into empty pairslot
		pairBlock.transform.SetParent(newParent);
		pairBlock.GetComponent<RectTransform>().anchoredPosition = Vector3.zero;
		int pairHand = (pairBlock.transform.parent.name[0] == 'R') ? 1 : -1;
		if (pairHand == 1) pairBlock.GetComponent<Image>().color = Color.red;
		else pairBlock.GetComponent<Image>().color = Color.blue;
		int pairID = Int32.Parse(pairBlock.name);
		if (mainInterface.gloves.ContainsKey(pairID)) 
		{
			mainInterface.gloves[pairID].GetComponent<HandController>().handedness = pairHand;
			mainInterface.gloves[pairID].GetComponent<HandController>().UpdateTexture();
			int pair = (int)Char.GetNumericValue(pairBlock.transform.parent.name[1]);
			mainInterface.gloves[pairID].transform.localPosition = glovePositions[pair] + pairHand * Vector3.right * gloveSeparation;
		}
		
		//update the orbit center
		mainInterface.UpdateOrbit();
	}
}
