using UnityEngine;

public class HighlightEffect:MonoBehaviour
{
    //the object to be highlighted
	public GameObject highlightObject;
    //the shader to use to emphasize the object
	public Shader highlightShader;
	
    //the mask corresponding to the layer that is reserved for highlighting
	private int highlightLayer;
    //the camera to use to render the highlighted object for shader use
	private Camera highlightCamera;
    //the camera component of this object
	private Camera thisCamera;
    //an unlit shader to render the object as a solid color
	private Shader unlitShader;
    //a material to use for blitting
	private Material highlightMaterial;

	void Start()
	{
		highlightLayer = LayerMask.NameToLayer("Highlight");
		thisCamera = GetComponent<Camera>();
		//add a new camera to render the highlighted object to before outlining it
		highlightCamera = new GameObject().AddComponent<Camera>();
        //camera should look at exactly the same area as the main camera
        highlightCamera.CopyFrom(thisCamera);
		highlightCamera.enabled = false;
		highlightCamera.name = "Highlight Camera";
		highlightCamera.transform.SetParent(transform);
		
        //make a material to apply the highlight shader using blitting
		unlitShader = Shader.Find("Unlit/Color");
		highlightMaterial = new Material(highlightShader);
	}
	
	void OnRenderImage(RenderTexture source, RenderTexture destination)
	{
		Graphics.Blit(source, destination);
		if (highlightObject != null)
		{
			//save the object's layer and shift it to the highlighting layer to be rendered separately
			int objectLayer = highlightObject.layer;
			highlightObject.layer = highlightLayer;
			Component[] transforms = highlightObject.GetComponentsInChildren(typeof(Transform));
			foreach (Transform t in transforms)
			{
				t.gameObject.layer = highlightLayer;
			}
			
			highlightCamera.CopyFrom(thisCamera);
			highlightCamera.clearFlags = CameraClearFlags.Color;
			highlightCamera.backgroundColor = Color.black;
			highlightCamera.cullingMask = 1 << highlightLayer;
			
			//create a separate render texture to output the isolated highlighted object to
			RenderTexture isolated = new RenderTexture(source.width, source.height, 0, RenderTextureFormat.R8);
			isolated.Create();

            highlightCamera.targetTexture = isolated;
			highlightCamera.RenderWithShader(unlitShader, "");
			highlightCamera.targetTexture = null;
			
			highlightMaterial.SetTexture("_SceneTex", source);
			Graphics.Blit(isolated, destination, highlightMaterial);
			
			isolated.Release();
			highlightObject.layer = objectLayer;
			foreach (Transform t in transforms)
			{
				t.gameObject.layer = objectLayer;
			}
		}
	}
}
