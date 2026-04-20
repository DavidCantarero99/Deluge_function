// BLOQUE 0 - Acceso y setup
numero_autorizado = "xxxxxx";
numero_visitante = visitor.get("phone");
operadores_destino = List();
// operadores_destino.add("asesor1@tuempresa.com");
// operadores_destino.add("asesor2@tuempresa.com");
logs = List();
logs.add("B0 | INICIO visitante=" + numero_visitante);
if(numero_visitante != numero_autorizado)
{
	resp_forward = Map();
	resp_forward.put("action","forward");
	msg_forward = List();
	msg_forward.add("Hola, te estamos conectando con uno de nuestros especialistas para brindarte una atencion personalizada de inmediato.");
	resp_forward.put("replies",msg_forward);
	if(!operadores_destino.isEmpty())
	{
		resp_forward.put("operators",operadores_destino);
	}
	logs.add("B0 | FORWARD no_autorizado");
	return resp_forward;
}
payload = Map();
payload.put("message",message.toMap());
payload.put("visitor",visitor.toMap());
// BLOQUE 1 - Deteccion de archivo en payload actual
attachments = message.get("attachments");
adjuntos = attachments;
if(adjuntos == null)
{
	adjuntos = List();
}
raw_adj_null = attachments == null;
raw_adj_count = 0;
if(attachments != null)
{
	raw_adj_count = attachments.size();
}
logs.add("B1 | raw attachments null=" + raw_adj_null + " count=" + raw_adj_count);
cond_if_attachments = false;
if(attachments != null)
{
	if(!attachments.isEmpty())
	{
		cond_if_attachments = true;
	}
}
logs.add("B1 | if(!attachments.isEmpty())=" + cond_if_attachments);
texto_mensaje = message.get("text");
tipo_mensaje = message.get("type");
es_archivo = false;
if(texto_mensaje != null && (texto_mensaje.contains("image/") || texto_mensaje.contains("audio/") || texto_mensaje.contains("File Shared")))
{
	es_archivo = true;
}
if(tipo_mensaje != null && (tipo_mensaje == "file" || tipo_mensaje == "files"))
{
	es_archivo = true;
}
adjunto_tiene_base64 = false;
if(!adjuntos.isEmpty())
{
	logs.add("B1 | if(!adjuntos.isEmpty())=true count=" + adjuntos.size());
	primer_adjunto = adjuntos.get(0);
	adjunto_tiene_base64 = primer_adjunto.get("base64") != null;
}
else
{
	logs.add("B1 | if(!adjuntos.isEmpty())=false");
}
necesita_enriquecer = es_archivo && (adjuntos.isEmpty() || !adjunto_tiene_base64);
logs.add("B1 | tipo=" + tipo_mensaje + " es_archivo=" + es_archivo + " adj_payload=" + adjuntos.size() + " need_enrich=" + necesita_enriquecer);
// BLOQUE 2 - Enriquecer desde evento actual (message.meta.urls)
if(necesita_enriquecer)
{
	meta_url_actual = "";
	meta_name_actual = "archivo";
	meta_mime_actual = "application/octet-stream";
	files_actual = message.get("files");
	if(files_actual != null)
	{
		if(!files_actual.isEmpty())
		{
			if(files_actual.get(0) != null)
			{
				meta_name_actual = files_actual.get(0).toString();
			}
		}
	}
	if(meta_name_actual != null)
	{
		nombre_l = meta_name_actual.toLowerCase();
		if(nombre_l.contains(".mp3") || nombre_l.contains(".mpeg") || nombre_l.contains(".mpga"))
		{
			meta_mime_actual = "audio/mpeg";
		}
		else if(nombre_l.contains(".ogg") || nombre_l.contains(".oga"))
		{
			meta_mime_actual = "audio/ogg";
		}
		else if(nombre_l.contains(".wav"))
		{
			meta_mime_actual = "audio/wav";
		}
		else if(nombre_l.contains(".m4a") || nombre_l.contains(".mp4"))
		{
			meta_mime_actual = "audio/mp4";
		}
		else if(nombre_l.contains(".jpg") || nombre_l.contains(".jpeg"))
		{
			meta_mime_actual = "image/jpeg";
		}
		else if(nombre_l.contains(".png"))
		{
			meta_mime_actual = "image/png";
		}
		else if(nombre_l.contains(".webp"))
		{
			meta_mime_actual = "image/webp";
		}
		else if(nombre_l.contains(".gif"))
		{
			meta_mime_actual = "image/gif";
		}
	}
	meta_obj = message.get("meta");
	if(meta_obj != null)
	{
		urls_meta = meta_obj.get("urls");
		meta_val = meta_obj.get("value");
		if(urls_meta != null)
		{
			if(meta_val != null)
			{
				meta_url_val = urls_meta.get(meta_val.toString());
				if(meta_url_val != null)
				{
					meta_url_actual = meta_url_val.toString();
				}
			}
			if(meta_url_actual == "")
			{
				for each  k_meta in urls_meta.keys()
				{
					v_meta = urls_meta.get(k_meta);
					if(v_meta != null)
					{
						if(v_meta.toString() != "")
						{
							meta_url_actual = v_meta.toString();
							break;
						}
					}
				}
			}
		}
	}
	meta_ok = false;
	if(meta_url_actual != "")
	{
		meta_ok = true;
	}
	logs.add("B2 | meta_name=" + meta_name_actual + " meta_url_ok=" + meta_ok);
	if(meta_ok)
	{
		try 
		{
			file_data_meta = invokeurl
			[
				url :meta_url_actual
				type :GET
				connection:"sales_cxn"
			];
			if(file_data_meta != null)
			{
				base64_meta = zoho.encryption.base64Encode(file_data_meta);
				if(base64_meta != null && base64_meta.length() > 0)
				{
					adj_map_meta = Map();
					adj_map_meta.put("url",meta_url_actual);
					adj_map_meta.put("name",meta_name_actual);
					adj_map_meta.put("base64",base64_meta);
					adj_map_meta.put("mime_type",meta_mime_actual);
					adjuntos = List();
					adjuntos.add(adj_map_meta);
					logs.add("B2 | meta_enrich_ok b64=" + base64_meta.length());
				}
				else
				{
					logs.add("B2 | meta_b64_empty");
				}
			}
			else
			{
				logs.add("B2 | meta_file_null");
			}
		}
		catch (e_meta)
		{
			logs.add("B2 | meta_err");
		}
	}
	else
	{
		logs.add("B2 | meta_url_missing");
		logs.add("B2F | start");
		visit_id_fb = visitor.get("visitid");
		logs.add("B2F | visitid=" + visit_id_fb);
		if(visit_id_fb != null)
		{
			url_conv_fb = "https://salesiq.zoho.com/api/v2/{nombre_empresa}/conversations?reference_ids=" + visit_id_fb.toString();
			conv_resp_fb = invokeurl
			[
				url :url_conv_fb
				type :GET
				connection:"salesiq"
			];
			conv_data_fb = conv_resp_fb.get("data");
			conv_count_fb = 0;
			if(conv_data_fb != null)
			{
				conv_count_fb = conv_data_fb.size();
			}
			logs.add("B2F | conv_count=" + conv_count_fb);
			if(conv_data_fb != null && conv_data_fb.size() > 0)
			{
				real_conv_id_fb = conv_data_fb.get(0).get("id").toString();
				conv_start_time_fb = conv_data_fb.get(0).get("start_time").toString();
				logs.add("B2F | conv_id=" + real_conv_id_fb);
				url_msgs_fb = "https://salesiq.zoho.com/api/v2/{nombre_empresa}/conversations/" + real_conv_id_fb + "/messages?sort_order=desc&from_time=" + conv_start_time_fb;
				mensajes_resp_fb = invokeurl
				[
					url :url_msgs_fb
					type :GET
					connection:"sales_cxn"
				];
				mensajes_data_fb = mensajes_resp_fb.get("data");
				msgs_count_fb = 0;
				if(mensajes_data_fb != null)
				{
					msgs_count_fb = mensajes_data_fb.size();
				}
				logs.add("B2F | msgs_count=" + msgs_count_fb);
				if(mensajes_data_fb != null && mensajes_data_fb.size() > 0)
				{
					expected_name = meta_name_actual;
					if(expected_name == null)
					{
						expected_name = "archivo";
					}
					selected_att_id = "";
					selected_file_name = "";
					selected_file_mime = "application/octet-stream";
					selected_file_url = "";
					selected_by_name = false;
					for each  msg_fb in mensajes_data_fb
					{
						if(msg_fb.get("type") == "file")
						{
							file_obj_fb = null;
							if(msg_fb.get("message") != null)
							{
								file_obj_fb = msg_fb.get("message").get("file");
							}
							if(file_obj_fb != null)
							{
								cand_att_id = "";
								cand_name = "archivo";
								cand_mime = "application/octet-stream";
								cand_url = "";
								if(file_obj_fb.get("attachment_id") != null)
								{
									cand_att_id = file_obj_fb.get("attachment_id").toString();
								}
								if(file_obj_fb.get("name") != null)
								{
									cand_name = file_obj_fb.get("name").toString();
								}
								if(file_obj_fb.get("type") != null)
								{
									cand_mime = file_obj_fb.get("type").toString();
								}
								if(file_obj_fb.get("url") != null)
								{
									cand_url_raw = file_obj_fb.get("url").toString();
									if(cand_url_raw.startsWith("/"))
									{
										cand_url = "https://salesiq.zoho.com" + cand_url_raw;
									}
									else
									{
										cand_url = cand_url_raw;
									}
								}
								selected_att_id = cand_att_id;
								selected_file_name = cand_name;
								selected_file_mime = cand_mime;
								selected_file_url = cand_url;
								if(expected_name != "archivo" && cand_name == expected_name)
								{
									selected_by_name = true;
									break;
								}
							}
						}
					}
					logs.add("B2F | pick by_name=" + selected_by_name + " att_id=" + selected_att_id + " name=" + selected_file_name);
					if(selected_att_id != "")
					{
						download_url_fb = "https://salesiq.zoho.com/api/v2/{nombre_empresa}/conversations/" + real_conv_id_fb + "/attachments/" + selected_att_id;
						logs.add("B2F | dl=attachment_v2");
						file_data_fb = invokeurl
						[
							url :download_url_fb
							type :GET
							connection:"salesiq"
						];
						if(file_data_fb != null)
						{
							base64_fb = zoho.encryption.base64Encode(file_data_fb);
							if(base64_fb != null && base64_fb.length() > 0)
							{
								adj_map_fb = Map();
								adj_map_fb.put("url",selected_file_url);
								adj_map_fb.put("name",selected_file_name);
								adj_map_fb.put("base64",base64_fb);
								adj_map_fb.put("mime_type",selected_file_mime);
								adjuntos = List();
								adjuntos.add(adj_map_fb);
								logs.add("B2F | ok b64=" + base64_fb.length());
							}
							else
							{
								logs.add("B2F | b64_empty");
							}
						}
						else
						{
							logs.add("B2F | file_null");
						}
					}
					else
					{
						logs.add("B2F | pick_miss");
					}
				}
			}
		}
		else
		{
			logs.add("B2F | visitid_null");
		}
	}
}
// BLOQUE 3 - Empaque final y envio a n8n
if(!adjuntos.isEmpty())
{
	payload.put("attachments",adjuntos);
}
adj_count = 0;
if(adjuntos != null)
{
	adj_count = adjuntos.size();
}
logs.add("B3 | FIN tipo=" + tipo_mensaje + " archivo=" + es_archivo + " enrich=" + necesita_enriquecer + " adj=" + adj_count);
if(adjuntos != null && !adjuntos.isEmpty())
{
	adj_fin = adjuntos.get(0);
	b64_len = 0;
	if(adj_fin.get("base64") != null)
	{
		b64_len = adj_fin.get("base64").toString().length();
	}
	logs.add("B3 | FIN_ADJ name=" + adj_fin.get("name") + " mime=" + adj_fin.get("mime_type") + " b64=" + b64_len);
}
else
{
	logs.add("B3 | FIN_ADJ none");
}
payload.put("debug_logs",logs);
respuesta_n8n = invokeurl
[
	url :"https://n8n.tuempresa.com/webhook/endpoint"
	type :POST
	parameters:payload.toString()
	headers:{"Content-Type":"application/json"}
];
if(respuesta_n8n == null || respuesta_n8n.isEmpty())
{
	resp_fallback = Map();
	resp_fallback.put("action","forward");
	msg_fallback = List();
	msg_fallback.add("Estoy experimentando un pequeno retraso. Te transferire con un asesor de inmediato.");
	resp_fallback.put("replies",msg_fallback);
	if(!operadores_destino.isEmpty())
	{
		resp_fallback.put("operators",operadores_destino);
	}
	return resp_fallback;
}
return respuesta_n8n;
