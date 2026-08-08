local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local targets=dofile(scripts_dir.."\\assault_targets.lua")
local out=mod_dir.."\\runtime\\reports\\assault-condition-correlation.tsv"
local done=false

local function valid(o)
 if o==nil then return false end
 local ok,v=pcall(function()return o:IsValid()end)
 return ok and v==true
end
local function write(rows)
 local f=io.open(out,"w"); if not f then return false end
 f:write("PlaceID\tKindID\tCID\tUID\tUIDName\tX\tY\tZ\tISAllFalse\tISAllTrue\tCallOKFalse\tCallOKTrue\tRawTypeFalse\tRawTypeTrue\n")
 for _,r in ipairs(rows)do
  f:write(tostring(r.place_id),"\t",tostring(r.kind_id),"\t",tostring(r.cid),"\t",tostring(r.uid),"\t",
   tostring(r.uid_name),"\t",tostring(r.x),"\t",tostring(r.y),"\t",tostring(r.z),"\t",
   tostring(r.v0),"\t",tostring(r.v1),"\t",tostring(r.ok0),"\t",tostring(r.ok1),"\t",
   tostring(r.type0),"\t",tostring(r.type1),"\n")
 end
 f:close(); return true
end
local function run(ctx)
 if done then return {status="complete",value_type="scan_state",value="already_completed",fingerprint="assault_condition:done"} end
 local util=StaticFindObject("/Script/DS.Default__DETUtil")
 local fn=StaticFindObject("/Script/DS.DETUtil:CUnexpectedMissionInStandAlone")
 local world=FindFirstOf("DClientQuestSystem")
 if not valid(world) then world=FindFirstOf("DPlayerController") end
 if not valid(util) or not valid(fn) or not valid(world) then
  return {status="unavailable",value_type="handles",value="known_query_not_ready",fingerprint="assault_condition:not_ready"}
 end
 local rows={}; local patterns={}
 for _,t in ipairs(targets)do
  local ok0,v0=pcall(function()return fn(util,world,t.place_id,false)end)
  local ok1,v1=pcall(function()return fn(util,world,t.place_id,true)end)
  local b0=ok0 and type(v0)=="boolean"; local b1=ok1 and type(v1)=="boolean"
  local sv0="ERR"; local sv1="ERR"
  if b0 then sv0=v0 end
  if b1 then sv1=v1 end
  rows[#rows+1]={place_id=t.place_id,kind_id=t.kind_id,cid=t.cid,uid=t.uid,uid_name=t.uid_name,
   x=t.x,y=t.y,z=t.z,v0=sv0,v1=sv1,ok0=b0,ok1=b1,type0=type(v0),type1=type(v1)}
  local key=tostring(sv0).."/"..tostring(sv1); patterns[key]=(patterns[key] or 0)+1
 end
 write(rows)
 local ps={}; for k,v in pairs(patterns)do ps[#ps+1]=k.."="..tostring(v) end; table.sort(ps)
 ctx.log("ASSAULT_CONDITION_CORRELATION",{rows=#rows,patterns=table.concat(ps,","),known_read_only_function="CUnexpectedMissionInStandAlone"})
 done=true
 return {status="ok",value_type="rows",value=tostring(#rows),fingerprint="assault_condition:"..table.concat(ps,",")}
end
return {id="assault_condition_correlation",enabled=true,description="One-shot 40-ID query through known read-only CUnexpectedMissionInStandAlone.",steps={{kind="custom",name="assault_condition_snapshot",class="<DETUtil>",role="assault_dynamic_correlation",purpose="known_read_only_query",cadence=1,once_per_session=true,run=run}}}
