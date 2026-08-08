local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_dir=mod_dir.."\\runtime\\reports"

local done=false

local class_names={
 "DInteractableComponent",
 "DsAnimationProp",
 "DsPickupProp",
 "DPropDataTable",
}

local function valid(o)
 if o==nil then return false end
 local ok,v=pcall(function()return o:IsValid()end)
 return ok and v==true
end

local function clean(v)
 return tostring(v or ""):gsub("\t"," "):gsub("[\r\n]"," ")
end

local function fname(o)
 local v="";pcall(function()v=o:GetFName():ToString()end);return tostring(v or "")
end
local function full(o)
 local v="";pcall(function()v=o:GetFullName()end);return tostring(v or "")
end
local function cname(o)
 local v="";pcall(function()v=o:GetClass():GetFullName()end);return tostring(v or "")
end
local function off(o)
 local v=-1;pcall(function()v=o:GetOffset_Internal()end);return tonumber(v) or -1
end

local function write_tsv(path,headers,rows)
 local f=io.open(path,"w");if not f then return false end
 f:write(table.concat(headers,"\t"),"\n")
 for _,r in ipairs(rows)do
  local vals={}
  for _,h in ipairs(headers)do vals[#vals+1]=clean(r[h]) end
  f:write(table.concat(vals,"\t"),"\n")
 end
 f:close();return true
end

local function inventory_instance(label,obj,function_rows,property_rows,evidence_rows)
 evidence_rows[#evidence_rows+1]={
  Class=label,Found=tostring(valid(obj)),
  Object=valid(obj) and full(obj) or ""
 }
 if not valid(obj) then return end

 local cls=nil;pcall(function()cls=obj:GetClass()end)
 if cls==nil then return end

 pcall(function()
  cls:ForEachFunction(function(fn)
   local props={}
   pcall(function()
    fn:ForEachProperty(function(p)
     props[#props+1]={n=fname(p),t=cname(p),o=off(p)}
    end)
   end)
   table.sort(props,function(a,b)
    if a.o~=b.o then return a.o<b.o end
    return a.n<b.n
   end)
   local args={};local ret=""
   for _,p in ipairs(props)do
    if string.lower(p.n)=="returnvalue" then
     ret=p.t
    else
     args[#args+1]=p.n..":"..p.t.."@"..tostring(p.o)
    end
   end
   function_rows[#function_rows+1]={
    Class=label,FunctionName=fname(fn),FunctionPath=full(fn),
    Parameters=table.concat(args,";"),ReturnType=ret
   }
  end)
 end)

 pcall(function()
  cls:ForEachProperty(function(p)
   property_rows[#property_rows+1]={
    Class=label,PropertyName=fname(p),PropertyType=cname(p),Offset=off(p)
   }
  end)
 end)
end

local function find_specific(name)
 local obj=nil
 pcall(function()obj=FindFirstOf(name)end)
 if valid(obj) then return obj end
 pcall(function()obj=StaticFindObject("/Script/DS.Default__"..name)end)
 return obj
end

local function run(ctx)
 if done then
  return {status="complete",value_type="scan_state",value="already_completed",fingerprint="treasure_actor_state_chain:done"}
 end

 local function_rows={}
 local property_rows={}
 local evidence_rows={}

 for _,name in ipairs(class_names)do
  inventory_instance(name,find_specific(name),function_rows,property_rows,evidence_rows)
 end

 -- Probe likely TreasureBox-named concrete classes from a small explicit list.
 -- No global actor scan and no unknown function invocation.
 local treasure_names={
  "DTreasureBox",
  "DTreasureBoxActor",
  "DsTreasureBox",
  "DsTreasureBoxActor",
  "DPropTreasureBox",
 }
 for _,name in ipairs(treasure_names)do
  inventory_instance(name,find_specific(name),function_rows,property_rows,evidence_rows)
 end

 write_tsv(
  report_dir.."\\treasure-actor-state-functions.tsv",
  {"Class","FunctionName","FunctionPath","Parameters","ReturnType"},
  function_rows
 )
 write_tsv(
  report_dir.."\\treasure-actor-state-properties.tsv",
  {"Class","PropertyName","PropertyType","Offset"},
  property_rows
 )
 write_tsv(
  report_dir.."\\treasure-actor-state-evidence.tsv",
  {"Class","Found","Object"},
  evidence_rows
 )

 ctx.log("TREASURE_ACTOR_STATE_CHAIN",{
  functions=#function_rows,
  properties=#property_rows,
  classes=#evidence_rows,
  unknown_functions_invoked=0,
  global_actor_scan=false,
  hooks=false
 })

 done=true
 return {
  status="ok",
  value_type="counts",
  value="functions="..tostring(#function_rows)..";properties="..tostring(#property_rows),
  fingerprint="treasure_actor_state_chain:"..tostring(#function_rows)..":"..tostring(#property_rows)
 }
end

return {
 id="treasure_actor_state_chain",
 enabled=true,
 description="Read-only TreasureBox/Prop/Interaction class signature inventory; no unknown calls or global actor scan.",
 steps={{kind="custom",name="treasure_actor_state_chain",class="<focused classes>",role="treasure_open_state_discovery",purpose="read_only_signature_inventory",cadence=1,once_per_session=true,run=run}}
}
