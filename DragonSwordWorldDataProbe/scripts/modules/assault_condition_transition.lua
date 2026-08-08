local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local targets=dofile(scripts_dir.."\\assault_targets.lua")

local current_path=mod_dir.."\\runtime\\reports\\assault-condition-current.tsv"
local history_path=mod_dir.."\\runtime\\reports\\assault-condition-history.tsv"
local batch_path=mod_dir.."\\runtime\\reports\\assault-condition-batches.tsv"

local batch_size=1
local next_index=1
local sequence=0
local disabled=false
local disable_reason=""
local state={}

local headers={
 "UTC","Sequence","Batch","Event","PlaceID","KindID","CID","UID","UIDName",
 "X","Y","Z","InStandAlone","CallOK","RawType","Changed"
}

local function valid(o)
 if o==nil then return false end
 local ok,v=pcall(function()return o:IsValid()end)
 return ok and v==true
end

local function clean(v)
 return tostring(v==nil and "" or v):gsub("\t"," "):gsub("[\r\n]"," ")
end

local function write_current()
 local rows={}
 for _,t in ipairs(targets)do
  local s=state[t.place_id]
  if s then
   rows[#rows+1]={
    UTC=s.utc,Sequence=s.sequence,Batch=s.batch,Event="current",
    PlaceID=t.place_id,KindID=t.kind_id,CID=t.cid,UID=t.uid,UIDName=t.uid_name,
    X=t.x,Y=t.y,Z=t.z,InStandAlone=s.value,CallOK=s.ok,RawType=s.raw_type,Changed=false
   }
  else
   rows[#rows+1]={
    UTC="",Sequence="",Batch="",Event="not_sampled",
    PlaceID=t.place_id,KindID=t.kind_id,CID=t.cid,UID=t.uid,UIDName=t.uid_name,
    X=t.x,Y=t.y,Z=t.z,InStandAlone="",CallOK="",RawType="",Changed=false
   }
  end
 end

 local f=io.open(current_path,"w")
 if not f then return false end
 f:write(table.concat(headers,"\t"),"\n")
 for _,r in ipairs(rows)do
  local values={};for _,h in ipairs(headers)do values[#values+1]=clean(r[h])end
  f:write(table.concat(values,"\t"),"\n")
 end
 f:close();return true
end

local function append_rows(path,rows)
 if #rows==0 then return true end
 local exists=false
 local c=io.open(path,"r");if c then exists=true;c:close()end
 local f=io.open(path,"a");if not f then return false end
 if not exists then f:write(table.concat(headers,"\t"),"\n")end
 for _,r in ipairs(rows)do
  local values={};for _,h in ipairs(headers)do values[#values+1]=clean(r[h])end
  f:write(table.concat(values,"\t"),"\n")
 end
 f:close();return true
end

local function append_batch(utc,seq,batch_no,start_index,end_index,status,detail)
 local exists=false
 local c=io.open(batch_path,"r");if c then exists=true;c:close()end
 local f=io.open(batch_path,"a");if not f then return false end
 if not exists then f:write("UTC\tSequence\tBatch\tStartIndex\tEndIndex\tStatus\tDetail\n")end
 f:write(
  clean(utc),"\t",clean(seq),"\t",clean(batch_no),"\t",
  clean(start_index),"\t",clean(end_index),"\t",
  clean(status),"\t",clean(detail),"\n"
 )
 f:close();return true
end

local function disable_session(reason,utc,seq,batch_no,start_index,end_index)
 disabled=true
 disable_reason=tostring(reason or "unknown")
 append_batch(utc,seq,batch_no,start_index,end_index,"disabled",disable_reason)
end

local function run(ctx)
 if disabled then
  return {
   status="complete",
   value_type="disabled",
   value=disable_reason,
   fingerprint="assault_batch:disabled:"..disable_reason
  }
 end

 sequence=sequence+1
 local utc=os.date("!%Y-%m-%dT%H:%M:%SZ")
 local batch_no=math.floor((next_index-1)/batch_size)+1
 local start_index=next_index
 local end_index=math.min(#targets,start_index+batch_size-1)

 ctx.phase("resolve_fresh_handles_batch_"..tostring(batch_no))

 local util=StaticFindObject("/Script/DS.Default__DETUtil")
 local fn=StaticFindObject("/Script/DS.DETUtil:CUnexpectedMissionInStandAlone")
 local world=FindFirstOf("DClientQuestSystem")
 if not valid(world) then world=FindFirstOf("DPlayerController") end

 if not valid(util) or not valid(fn) or not valid(world) then
  append_batch(utc,sequence,batch_no,start_index,end_index,"not_ready","fresh handles invalid")
  return {
   status="unavailable",
   value_type="handles",
   value="fresh_handles_not_ready",
   fingerprint="assault_batch:not_ready"
  }
 end

 local changes={}
 local call_count=0

 for index=start_index,end_index do
  -- Revalidate the context immediately before every ProcessEvent call.
  if not valid(world) or not valid(util) or not valid(fn) then
   disable_session("handle_became_invalid_before_call_"..tostring(index),utc,sequence,batch_no,start_index,end_index)
   write_current()
   return {
    status="partial",
    value_type="safety_stop",
    value=disable_reason,
    fingerprint="assault_batch:safety_stop:"..disable_reason
   }
  end

  local target=targets[index]
  ctx.phase("query_place_"..tostring(target.place_id))

  -- ISAll=false only. Previous captures showed false/true modes changing together,
  -- so the second call is removed to halve native ProcessEvent pressure.
  local ok,value=pcall(function()
   return fn(util,world,target.place_id,false)
  end)

  call_count=call_count+1

  if not ok or type(value)~="boolean" then
   local reason=not ok and ("pcall_failed_place_"..tostring(target.place_id))
    or ("non_boolean_place_"..tostring(target.place_id).."_type_"..type(value))
   disable_session(reason,utc,sequence,batch_no,start_index,end_index)
   write_current()

   ctx.log("ASSAULT_BATCH_SAFETY_STOP",{
    place_id=target.place_id,
    reason=reason,
    calls_this_batch=call_count
   })

   return {
    status="partial",
    value_type="safety_stop",
    value=reason,
    fingerprint="assault_batch:safety_stop:"..reason
   }
  end

  local old=state[target.place_id]
  local changed=old~=nil and old.value~=value
  local event=old==nil and "baseline" or (changed and "changed" or "current")

  state[target.place_id]={
   value=value,ok=true,raw_type=type(value),utc=utc,sequence=sequence,batch=batch_no
  }

  if old==nil or changed then
   changes[#changes+1]={
    UTC=utc,Sequence=sequence,Batch=batch_no,Event=event,
    PlaceID=target.place_id,KindID=target.kind_id,CID=target.cid,UID=target.uid,
    UIDName=target.uid_name,X=target.x,Y=target.y,Z=target.z,
    InStandAlone=value,CallOK=true,RawType=type(value),Changed=changed
   }
  end
 end

 append_rows(history_path,changes)
 write_current()
 append_batch(
  utc,sequence,batch_no,start_index,end_index,"ok",
  "calls="..tostring(call_count).."; mode=ISAll_false_only"
 )

 next_index=end_index+1
 if next_index>#targets then next_index=1 end

 local true_count=0
 local sampled=0
 for _,s in pairs(state)do
  sampled=sampled+1
  if s.value==true then true_count=true_count+1 end
 end

 ctx.log("ASSAULT_CONDITION_BATCH",{
  batch=batch_no,
  start_index=start_index,
  end_index=end_index,
  calls=call_count,
  changes=#changes,
  sampled_total=sampled,
  true_total=true_count,
  next_index=next_index,
  mode="ISAll_false_only"
 })

 return {
  status="ok",
  value_type="batch",
  value="batch="..tostring(batch_no)..";calls="..tostring(call_count)..";changes="..tostring(#changes),
  fingerprint="assault_batch:"..tostring(batch_no)..":"..tostring(sampled)..":"..tostring(true_count)
 }
end

return {
 id="assault_condition_transition",
 enabled=false,
 description="Archived UnexpectedMission trigger-state correlation. The trigger-state semantics are confirmed, so the module is disabled and excluded from the automatic probe order.",
 steps={{
  kind="custom",
  name="assault_condition_safe_batch",
  class="<DETUtil>",
  role="assault_dynamic_semantics",
  purpose="known_read_only_batched_query",
  cadence=1,
  run=run
 }}
}
