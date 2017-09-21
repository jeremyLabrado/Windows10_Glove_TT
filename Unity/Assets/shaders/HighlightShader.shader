Shader "Hidden/Highlight"
{
	Properties
	{
		_MainTex("Main Texture",2D) = "black"{}
		_SceneTex("Scene Texture",2D) = "black"{}
	}
	SubShader
	{
		Pass
		{
			CGPROGRAM
			
			#pragma vertex vert
			#pragma fragment frag
			#include "UnityCG.cginc"
			
			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};
			
			struct v2f
			{
				float4 pos : SV_POSITION;
				float2 uv : TEXCOORD0;
			};
			
			sampler2D _MainTex;
			float2 _MainTex_TexelSize;
			
			v2f vert(appdata v)
			{
				v2f o;
				
				o.pos = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;
				
				return o;
			}
			
			float4 frag(v2f i) : COLOR
			{
				float intensity = 0.0;
				int depth = 15;
				
				for (int x = 0; x < depth; x++)
				{
					intensity += tex2D(_MainTex, i.uv.xy + float2((x-depth/2)*_MainTex_TexelSize.x, 0)).r/depth;
				}
				
				float4 col = intensity;
				return col;
			}
			
			ENDCG
		}
		
		GrabPass{}
		
		Pass
		{
			CGPROGRAM
			
			#pragma vertex vert
			#pragma fragment frag
			#include "UnityCG.cginc"
			
			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
			};
			
			struct v2f
			{
				float4 pos : SV_POSITION;
				float2 uv : TEXCOORD0;
				float2 uvOriginal : TEXCOORD1;
			};
			
			sampler2D _MainTex;
			sampler2D _GrabTexture;
			float2 _GrabTexture_TexelSize;
			sampler2D _SceneTex;
			
			v2f vert(appdata v)
			{
				v2f o;
				
				o.pos = UnityObjectToClipPos(v.vertex);
				o.uv = ComputeGrabScreenPos(o.pos);
				o.uvOriginal = v.uv;
				
				return o;
			}
			
			float4 frag(v2f i) : COLOR
			{
				float intensity = 0.0;
				int depth = 15;
				
				if (tex2D(_MainTex, i.uvOriginal.xy).r > 0)
				{
					return tex2D(_SceneTex, i.uvOriginal);
				}
				
				for (int y = 0; y < depth; y++)
				{
					intensity += tex2D(_GrabTexture, i.uv.xy + float2(0, (y-depth/2)*_GrabTexture_TexelSize.y)).r/depth;
				}
				
				float4 col = intensity*float4(0.0,0.680,0.934,1.0)*2 + (1-intensity)*tex2D(_SceneTex,float2(i.uvOriginal.x,i.uvOriginal.y));
				return col;
			}
			
			ENDCG
		}
	}
}