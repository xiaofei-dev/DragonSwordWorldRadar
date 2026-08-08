local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_dir=mod_dir.."\\runtime\\reports"
local catalog=dofile(scripts_dir.."\\treasure_real_actor_catalog.lua")

local current_path=report_dir.."\\treasure-proximity-current-v2.tsv"
local history_path=report_dir.."\\treasure-proximity-history-v2.tsv"
local class_path=report_dir.."\\treasure-real-class-current-v2.tsv"
local actor_path=report_dir.."\\treasure-real-actor-current.tsv"

local player_radius=2500.0
local match_radius_xy=600.0
local match_radius_z=600.0
local loaded_zone_radius=10000.0
local suspect_samples=3
local suspect_seconds=2
local max_class_instances=512

local classes={
 "TreasureBox01_C","TreasureBox02_C","TreasureBox03_C","TreasureBox04_C",
 "TreasureBox05_C","TreasureBox06_C","TreasureBox04Key_C",
 "TreasureBox02_Mount_C","TreasureBox03_Mount_C","TreasureBox05_Mount_C",
 "TreasureBox03_OnlyFront_C"
}

local catalog_by_class={}
for _,point in ipairs(catalog)do
 local list=catalog_by_class[point.class_name]
 if not list then list={};catalog_by_class[point.class_name]=list end
 list[#list+1]=point
end

local state={}
local sample_sequence=0
local class_cache={}
local class_status={}
local next_background_class=1
local cache_max_age_samples=4

local function valid(o)
 if o==nil then return false end
 local ok,v=pcall(function()return o:IsValid()end)
 return ok and v==true
end

local function number(v)
 if type(v)=="number" then return v end
 return tonumber(tostring(v or ""))
end

local function actor_location(actor)
 if not valid(actor) then return nil,nil,nil end
 local loc=nil
 pcall(function()loc=actor:K2_GetActorLocation()end)
 if loc==nil then pcall(function()loc=actor:GetActorLocation()end)end
 if loc==nil then return nil,nil,nil end
 local x=nil;local y=nil;local z=nil
 pcall(function()x=number(loc.X)end)
 pcall(function()y=number(loc.Y)end)
 pcall(function()z=number(loc.Z)end)
 return x,y,z
end

local function player_location()
 local pawn=nil
 pcall(function()pawn=FindFirstOf("DPlayerCharacter")end)
 if not valid(pawn) then pcall(function()pawn=FindFirstOf("Character")end)end
 if not valid(pawn) then return nil,nil,nil end
 return actor_location(pawn)
end

local function dist_xy(ax,ay,bx,by)
 local dx=ax-bx;local dy=ay-by
 return math.sqrt(dx*dx+dy*dy)
end

local function dist3(ax,ay,az,bx,by,bz)
 local dxy=dist_xy(ax,ay,bx,by)
 local dz=(az or 0)-(bz or 0)
 return math.sqrt(dxy*dxy+dz*dz)
end

local function clean(v)
 return tostring(v==nil and "" or v):gsub("\t"," "):gsub("[\r\n]"," ")
end

local proximity_headers={
 "UTC","Sequence","Event","UID","UIDName","CID","PropID","ClassName",
 "TreasureX","TreasureY","TreasureZ","PlayerDistance",
 "ActorPresent","ActorDistanceXY","ActorDistanceZ","CandidateCount",
 "ClassLoadedCount","ZoneActorCount","Observable","SuspectMissCount",
 "AbsenceSeconds","EverPresent","Reappeared","InferredState","Confidence"
}

local function write_rows(path,headers,rows,append)
 local exists=false
 if append then local c=io.open(path,"r");if c then exists=true;c:close()end end
 local f=io.open(path,append and "a" or "w");if not f then return false end
 if not exists then f:write(table.concat(headers,"\t"),"\n")end
 for _,r in ipairs(rows)do
  local vals={};for _,h in ipairs(headers)do vals[#vals+1]=clean(r[h])end
  f:write(table.concat(vals,"\t"),"\n")
 end
 f:close();return true
end

local function nearest_catalog(actor)
 local best=nil
 local best_xy=nil
 local best_z=nil
 for _,point in ipairs(catalog_by_class[actor.class_name] or {})do
  local dxy=dist_xy(actor.x,actor.y,point.x,point.y)
  local dz=math.abs((actor.z or 0)-(point.z or 0))
  if best_xy==nil or dxy<best_xy or (dxy==best_xy and dz<best_z)then
   best=point;best_xy=dxy;best_z=dz
  end
 end
 return best,best_xy,best_z
end

local function write_actor_rows(utc,actors,px,py,pz)
 local headers={
  "UTC","Sequence","ClassName","ClassIndex","ActorX","ActorY","ActorZ",
  "PlayerDistance","NearestUID","NearestUIDName","NearestCID","NearestPropID",
  "NearestTreasureX","NearestTreasureY","NearestTreasureZ",
  "MatchDistanceXY","MatchDistanceZ","WithinExactMatch"
 }
 local rows={}
 local indexes={}
 for _,actor in ipairs(actors)do
  indexes[actor.class_name]=(indexes[actor.class_name] or 0)+1
  local point,dxy,dz=nearest_catalog(actor)
  rows[#rows+1]={
   UTC=utc,Sequence=sample_sequence,ClassName=actor.class_name,
   ClassIndex=indexes[actor.class_name],ActorX=actor.x,ActorY=actor.y,ActorZ=actor.z,
   PlayerDistance=px and dist3(px,py,pz,actor.x,actor.y,actor.z) or "",
   NearestUID=point and point.uid or "",NearestUIDName=point and point.uid_name or "",
   NearestCID=point and point.cid or "",NearestPropID=point and point.prop_id or "",
   NearestTreasureX=point and point.x or "",NearestTreasureY=point and point.y or "",
   NearestTreasureZ=point and point.z or "",MatchDistanceXY=dxy or "",MatchDistanceZ=dz or "",
   WithinExactMatch=point~=nil and dxy<=match_radius_xy and dz<=match_radius_z or false,
  }
 end
 table.sort(rows,function(a,b)
  if a.ClassName~=b.ClassName then return a.ClassName<b.ClassName end
  return a.ClassIndex<b.ClassIndex
 end)
 write_rows(actor_path,headers,rows,false)
end

local function scan_classes(utc,selected)
 local class_rows={}
 local counts={}
 for _,class_name in ipairs(classes)do
  if selected[class_name] then
   local list=nil
   local ok,ret=pcall(function()return FindAllOf(class_name)end)
   if ok and type(ret)=="table" then list=ret else list={} end
   local count=math.min(#list,max_class_instances)
   local sampled={}
   for i=1,count do
    local actor=list[i]
    if valid(actor) then
     local x,y,z=actor_location(actor)
     if x and y then
      sampled[#sampled+1]={class_name=class_name,x=x,y=y,z=z}
     end
    end
   end
   class_cache[class_name]=sampled
   class_status[class_name]={sequence=sample_sequence,returned=#list,scanned=count}
  end
  local status=class_status[class_name]
  local age=status and sample_sequence-status.sequence or ""
  class_rows[#class_rows+1]={
   UTC=utc,Sequence=sample_sequence,Class=class_name,
   Returned=status and status.returned or "",Scanned=status and status.scanned or "",
   SampledNow=selected[class_name] or false,CacheAgeSamples=age,
  }
 end

 local actors={}
 for _,class_name in ipairs(classes)do
  local status=class_status[class_name]
  if status and sample_sequence-status.sequence<=cache_max_age_samples then
   for _,actor in ipairs(class_cache[class_name] or {})do actors[#actors+1]=actor end
  end
  counts[class_name]=status and sample_sequence-status.sequence<=cache_max_age_samples
   and #(class_cache[class_name] or {}) or 0
 end
 write_rows(class_path,
  {"UTC","Sequence","Class","Returned","Scanned","SampledNow","CacheAgeSamples"},
  class_rows,false)
 return actors,counts
end

local function run(ctx)
 if type(FindAllOf)~="function" then
  return {status="unsupported",value_type="FindAllOf",value="unavailable",fingerprint="treasure_real:no_findall"}
 end

 sample_sequence=sample_sequence+1
 local utc=os.date("!%Y-%m-%dT%H:%M:%SZ")
 local now=os.time()
 local px,py,pz=player_location()

 local selected={}
 local nearby_class_count=0
 if px and py then
  for _,point in ipairs(catalog)do
   if dist3(px,py,pz,point.x,point.y,point.z)<=player_radius
    and not selected[point.class_name]
   then
    selected[point.class_name]=true
    nearby_class_count=nearby_class_count+1
   end
  end
 end
 if nearby_class_count==0 then
  selected[classes[next_background_class]]=true
  next_background_class=next_background_class+1
  if next_background_class>#classes then next_background_class=1 end
 end

 -- Scan only nearby required classes, or one rotating class while no target is nearby.
 ctx.phase("scan_exact_treasure_classes")
 local actors,class_counts=scan_classes(utc,selected)
 write_actor_rows(utc,actors,px,py,pz)

 if not px or not py then
  ctx.log("TREASURE_REAL_ACTOR_SNAPSHOT",{
   sequence=sample_sequence,exact_class_actors=#actors,player_ready=false,
   classes_sampled=1,classes=#classes,global_actor_scan=false,
  })
  return {
   status="ok",value_type="actor_snapshot",
   value="player_not_ready;actors="..#actors,
   fingerprint="treasure_real:actors_only:"..#actors,
  }
 end

 ctx.phase("evaluate_nearby_catalog_points")
 local zone_actor_count=0
 for _,actor in ipairs(actors)do
  local _,mapped_xy,mapped_z=nearest_catalog(actor)
  if mapped_xy and mapped_xy<=match_radius_xy and mapped_z<=match_radius_z
   and dist3(px,py,pz,actor.x,actor.y,actor.z)<=loaded_zone_radius
  then
   zone_actor_count=zone_actor_count+1
  end
 end

 local rows={}
 local changes={}
 for _,point in ipairs(catalog)do
  local pd=dist3(px,py,pz,point.x,point.y,point.z)
  if pd<=player_radius then
   local best_xy=nil
   local best_z=nil
   local candidates=0
   for _,actor in ipairs(actors)do
    if actor.class_name==point.class_name then
     local dxy=dist_xy(actor.x,actor.y,point.x,point.y)
     local dz=math.abs((actor.z or 0)-(point.z or 0))
     if dxy<=match_radius_xy and dz<=match_radius_z then
      candidates=candidates+1
      if best_xy==nil or dxy<best_xy or (dxy==best_xy and dz<best_z)then
       best_xy=dxy;best_z=dz
      end
     end
    end
   end

   local key=tostring(point.uid_name).."|"..tostring(point.cid).."|"..tostring(point.prop_id)
   local old=state[key] or {
    miss=0,inferred="unknown",present=nil,ever_present=false,first_missing=nil,
   }
   local present=candidates==1
   local ambiguous=candidates>1
   local reappeared=present and old.present==false
   local observable=present or old.ever_present or zone_actor_count>0
   local miss=old.miss
   local first_missing=old.first_missing
   local ever_present=old.ever_present or present

   if present then
    miss=0;first_missing=nil
   elseif ambiguous or not observable then
    miss=0;first_missing=nil
   else
    miss=miss+1
    if not first_missing then first_missing=now end
   end

   local absence_seconds=first_missing and math.max(0,now-first_missing) or 0
   local inferred="unknown"
   local confidence="low"
   if ambiguous then
    inferred="ambiguous_actor_match"
   elseif present then
    inferred="unopened_present"
    confidence="high"
   elseif not observable then
    inferred="absence_unobservable"
   elseif ever_present and miss>=2 and absence_seconds>=1 then
    inferred="opened_after_observed_presence"
    confidence="strong_diagnostic"
   elseif miss>=suspect_samples and absence_seconds>=suspect_seconds then
    inferred="selective_absence_observed"
    confidence="diagnostic"
   else
    inferred="pending_absence_confirmation"
   end

   local changed=old.present==nil or old.present~=present or old.inferred~=inferred or reappeared
   local row={
    UTC=utc,Sequence=sample_sequence,
    Event=changed and (reappeared and "reappeared" or (old.present==nil and "baseline" or "changed")) or "current",
    UID=point.uid,UIDName=point.uid_name,CID=point.cid,PropID=point.prop_id,ClassName=point.class_name,
    TreasureX=point.x,TreasureY=point.y,TreasureZ=point.z,PlayerDistance=pd,
    ActorPresent=present,ActorDistanceXY=best_xy or "",ActorDistanceZ=best_z or "",
    CandidateCount=candidates,ClassLoadedCount=class_counts[point.class_name] or 0,
    ZoneActorCount=zone_actor_count,Observable=observable,SuspectMissCount=miss,
    AbsenceSeconds=absence_seconds,EverPresent=ever_present,Reappeared=reappeared,
    InferredState=inferred,Confidence=confidence,
   }
   rows[#rows+1]=row
   if changed then changes[#changes+1]=row end
   state[key]={
    miss=miss,inferred=inferred,present=present,ever_present=ever_present,
    first_missing=first_missing,
   }
  end
 end

 table.sort(rows,function(a,b)return tonumber(a.PlayerDistance)<tonumber(b.PlayerDistance)end)
 write_rows(current_path,proximity_headers,rows,false)
 if #changes>0 then write_rows(history_path,proximity_headers,changes,true) end

 local present_count=0
 local opened_after_presence=0
 local selective_absence=0
 for _,row in ipairs(rows)do
  if row.ActorPresent==true then present_count=present_count+1 end
  if row.InferredState=="opened_after_observed_presence" then opened_after_presence=opened_after_presence+1 end
  if row.InferredState=="selective_absence_observed" then selective_absence=selective_absence+1 end
 end

 ctx.log("TREASURE_REAL_ACTOR_PROXIMITY",{
  sequence=sample_sequence,nearby_points=#rows,exact_class_actors=#actors,
  zone_actor_count=zone_actor_count,present=present_count,
  opened_after_presence=opened_after_presence,selective_absence=selective_absence,
  changes=#changes,player_radius=player_radius,match_radius_xy=match_radius_xy,
  match_radius_z=match_radius_z,classes=#classes,global_actor_scan=false,
  classes_sampled=nearby_class_count>0 and nearby_class_count or 1,
 })

 return {
  status="ok",value_type="counts",
  value="nearby="..#rows..";present="..present_count..";opened_after_presence="..opened_after_presence..";selective_absence="..selective_absence,
  fingerprint="treasure_real:"..#rows..":"..present_count..":"..opened_after_presence..":"..selective_absence,
 }
end

return {
 id="treasure_real_actor_proximity",
 enabled=false,
 interval_ms=500,
 description="Archived Treasure diagnostic sampler. Evidence and implementation are preserved, but the module is disabled and excluded from the automatic probe order.",
 steps={{
  kind="custom",name="treasure_real_actor_proximity",
  class="<11 exact TreasureBox generated classes>",
  role="treasure_opened_state_inference",
  purpose="exact_actor_identity_presence_absence",
  cadence=1,run=run,
 }}
}
