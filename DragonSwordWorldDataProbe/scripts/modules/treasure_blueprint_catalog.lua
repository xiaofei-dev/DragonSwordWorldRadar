local source=debug.getinfo(1,"S").source
local file=type(source)=="string" and source:sub(1,1)=="@" and source:sub(2) or nil
local module_dir=file and file:match("^(.*)[/\\][^/\\]+$") or "."
local scripts_dir=module_dir:match("^(.*)[/\\][^/\\]+$") or "."
local mod_dir=scripts_dir:match("^(.*)[/\\][^/\\]+$") or "."
local report_dir=mod_dir.."\\runtime\\reports"

local catalog_path=report_dir.."\\treasure-blueprint-catalog.tsv"
local blueprint_path=report_dir.."\\treasure-blueprint-types.tsv"
local access_path=report_dir.."\\treasure-blueprint-access.tsv"
local live_path=report_dir.."\\treasure-blueprint-live-instances.tsv"

local static_done=false
local last_live_fingerprint=""

local function unwrap(v)
 if v==nil then return nil end
 local ok,t=pcall(function()return v:type()end)
 if ok and (t=="RemoteUnrealParam" or t=="LocalUnrealParam") then
  local ok2,g=pcall(function()return v:get()end)
  if ok2 then return g end
 end
 return v
end

local function valid(o)
 o=unwrap(o)
 if o==nil then return false end
 local ok,v=pcall(function()return o:IsValid()end)
 return ok and v==true
end

local function scalar(v)
 v=unwrap(v)
 if v==nil then return "" end
 if type(v)=="string" or type(v)=="number" or type(v)=="boolean" then
  return tostring(v)
 end
 local ok,s=pcall(function()return v:ToString()end)
 if ok and s~=nil then return tostring(s) end
 return tostring(v)
end

local function field(o,name)
 o=unwrap(o)
 if o==nil then return nil end
 local ok,v=pcall(function()return o[name]end)
 if ok and v~=nil then return unwrap(v) end
 local ok2,v2=pcall(function()return o:GetPropertyValue(name)end)
 if ok2 then return unwrap(v2) end
 return nil
end

local function full(o)
 o=unwrap(o)
 if o==nil then return "" end
 local s=""
 pcall(function()s=o:GetFullName()end)
 return tostring(s or "")
end

local function typename(v)
 v=unwrap(v)
 if v==nil then return "nil" end
 local ok,t=pcall(function()return v:type()end)
 if ok then return tostring(t) end
 return type(v)
end

local function clean(v)
 return tostring(v or ""):gsub("\t"," "):gsub("[\r\n]"," ")
end

local function write(path,headers,rows)
 local f=io.open(path,"w");if not f then return false end
 f:write(table.concat(headers,"\t"),"\n")
 for _,r in ipairs(rows)do
  local values={}
  for _,h in ipairs(headers)do values[#values+1]=clean(r[h]) end
  f:write(table.concat(values,"\t"),"\n")
 end
 f:close();return true
end

local function find_prop_table()
 local attempts={}

 local function test(label,fn)
  local ok,v=pcall(fn)
  v=unwrap(v)
  attempts[#attempts+1]={Method=label,OK=ok,Valid=valid(v),Object=valid(v) and full(v) or "",ValueType=typename(v)}
  if ok and valid(v) then return v end
  return nil
 end

 local value=test("FindFirstOf_DPropDataTable",function()return FindFirstOf("DPropDataTable")end)
 if valid(value) then return value,attempts end

 value=test("CDO_DPropDataTable",function()return StaticFindObject("/Script/DS.Default__DPropDataTable")end)
 if valid(value) then return value,attempts end

 for _,manager_name in ipairs({"GameDBTableManager","DGameDBTableManager"})do
  local manager=test("FindFirstOf_"..manager_name,function()return FindFirstOf(manager_name)end)
  if valid(manager) then
   local table_value=nil
   local ok,pv=pcall(function()return manager.DPropDataTable end)
   pv=unwrap(pv)
   attempts[#attempts+1]={
    Method=manager_name..".DPropDataTable",OK=ok,Valid=valid(pv),
    Object=valid(pv) and full(pv) or "",ValueType=typename(pv)
   }
   if ok and valid(pv) then table_value=pv end
   if valid(table_value) then return table_value,attempts end
  end
 end

 return nil,attempts
end

local function blueprint_basename(path)
 path=tostring(path or ""):gsub("\\","/")
 local tail=path:match("([^/]+)$") or path
 local base=tail:match("^([^%.]+)") or tail
 return base
end

local function derive_class_names(path)
 local base=blueprint_basename(path)
 if base=="" then return "", "" end
 return base, base.."_C"
end

local function collect_static(ctx)
 local prop,attempts=find_prop_table()

 write(access_path,{"Method","OK","Valid","Object","ValueType"},attempts)

 if not valid(prop) then
  return nil,"DPropDataTable_not_found"
 end

 ctx.phase("read_TreasureBoxPropDataMap")
 local wrapper=field(prop,"TreasureBoxPropDataMap")
 local wrapper_type=typename(wrapper)

 ctx.phase("read_TreasureBoxPropDataMap_Data")
 local data=field(wrapper,"Data")
 local data_type=typename(data)

 local access_rows=attempts
 access_rows[#access_rows+1]={Method="TreasureBoxPropDataMap",OK=true,Valid=true,Object="",ValueType=wrapper_type}
 access_rows[#access_rows+1]={Method="TreasureBoxPropDataMap.Data",OK=true,Valid=true,Object="",ValueType=data_type}
 write(access_path,{"Method","OK","Valid","Object","ValueType"},access_rows)

 if data==nil then
  return nil,"Data_map_unavailable"
 end

 local rows={}
 local ok,err=pcall(function()
  data:ForEach(function(key,value)
   key=unwrap(key);value=unwrap(value)

   local id=tonumber(scalar(field(value,"ID"))) or tonumber(scalar(key)) or 0
   local bp=scalar(field(value,"BlueprintPath"))
   local base,class_name=derive_class_names(bp)

   rows[#rows+1]={
    Key=scalar(key),
    ID=id,
    Memo=scalar(field(value,"Memo")),
    Name=scalar(field(value,"Name")),
    Grade=scalar(field(value,"Grade")),
    OpenType=scalar(field(value,"OpenType")),
    OpenValue01=scalar(field(value,"OpenValue01")),
    OpenValue02=scalar(field(value,"OpenValue02")),
    FogSectionID=scalar(field(value,"FogSectionID")),
    BlueprintPath=bp,
    BlueprintBase=base,
    GeneratedClassShortName=class_name,
    ItemRewardID=scalar(field(value,"ItemRewardID")),
    TreasureBoxLockType=scalar(field(value,"TreasureBoxLockType")),
    IsOpenedInstantly=scalar(field(value,"IsOpenedInstantly")),
   }
  end)
 end)

 if not ok then
  return nil,"TMap_ForEach_failed:"..tostring(err)
 end

 table.sort(rows,function(a,b)
  if a.ID~=b.ID then return a.ID<b.ID end
  return a.BlueprintPath<b.BlueprintPath
 end)

 write(catalog_path,{
  "Key","ID","Memo","Name","Grade","OpenType","OpenValue01","OpenValue02",
  "FogSectionID","BlueprintPath","BlueprintBase","GeneratedClassShortName",
  "ItemRewardID","TreasureBoxLockType","IsOpenedInstantly"
 },rows)

 local counts={}
 for _,r in ipairs(rows)do
  if r.BlueprintPath~="" then
   local item=counts[r.BlueprintPath]
   if not item then
    item={
     BlueprintPath=r.BlueprintPath,
     BlueprintBase=r.BlueprintBase,
     GeneratedClassShortName=r.GeneratedClassShortName,
     RecordCount=0
    }
    counts[r.BlueprintPath]=item
   end
   item.RecordCount=item.RecordCount+1
  end
 end

 local blueprints={}
 for _,item in pairs(counts)do blueprints[#blueprints+1]=item end
 table.sort(blueprints,function(a,b)return a.BlueprintPath<b.BlueprintPath end)

 write(blueprint_path,{
  "BlueprintPath","BlueprintBase","GeneratedClassShortName","RecordCount"
 },blueprints)

 return {
  rows=rows,
  blueprints=blueprints,
  wrapper_type=wrapper_type,
  data_type=data_type,
 },nil
end

local function snapshot_live(blueprints)
 local rows={}
 local fingerprint_parts={}

 for _,bp in ipairs(blueprints or {})do
  local class_name=bp.GeneratedClassShortName
  local instance=nil
  if class_name~="" then
   pcall(function()instance=FindFirstOf(class_name)end)
  end

  local found=valid(instance)
  local cls=nil
  if found then pcall(function()cls=instance:GetClass()end) end

  local fn_count=0
  local prop_count=0
  if cls~=nil then
   pcall(function()cls:ForEachFunction(function(_)fn_count=fn_count+1 end)end)
   pcall(function()cls:ForEachProperty(function(_)prop_count=prop_count+1 end)end)
  end

  rows[#rows+1]={
   BlueprintPath=bp.BlueprintPath,
   GeneratedClassShortName=class_name,
   Found=found,
   Instance=found and full(instance) or "",
   FunctionCount=fn_count,
   PropertyCount=prop_count,
  }

  fingerprint_parts[#fingerprint_parts+1]=
   class_name.."="..tostring(found)..":"..tostring(fn_count)..":"..tostring(prop_count)
 end

 table.sort(rows,function(a,b)return a.BlueprintPath<b.BlueprintPath end)
 write(live_path,{
  "BlueprintPath","GeneratedClassShortName","Found","Instance","FunctionCount","PropertyCount"
 },rows)

 table.sort(fingerprint_parts)
 return rows,table.concat(fingerprint_parts,"|")
end

local static_cache=nil

local function run(ctx)
 if not static_done then
  ctx.phase("locate_DPropDataTable")
  local result,err=collect_static(ctx)
  if not result then
   ctx.log("TREASURE_BLUEPRINT_CATALOG_BLOCKED",{error=tostring(err)})
   return {
    status="unavailable",
    value_type="catalog",
    value=tostring(err),
    fingerprint="treasure_blueprint:blocked:"..tostring(err)
   }
  end
  static_cache=result
  static_done=true

  ctx.log("TREASURE_BLUEPRINT_CATALOG",{
   records=#result.rows,
   unique_blueprints=#result.blueprints,
   wrapper_type=result.wrapper_type,
   data_type=result.data_type,
   unknown_function_invocations=0
  })
 end

 ctx.phase("probe_loaded_blueprint_instances")
 local rows,fingerprint=snapshot_live(static_cache.blueprints)

 local loaded=0
 for _,r in ipairs(rows)do if r.Found==true then loaded=loaded+1 end end

 if fingerprint~=last_live_fingerprint then
  ctx.log("TREASURE_BLUEPRINT_LIVE_CHANGE",{
   loaded=loaded,total=#rows
  })
  last_live_fingerprint=fingerprint
 end

 return {
  status="ok",
  value_type="counts",
  value="catalog="..tostring(#static_cache.rows)..";blueprints="..
   tostring(#static_cache.blueprints)..";loaded="..tostring(loaded),
  fingerprint="treasure_blueprint_live:"..fingerprint
 }
end

return {
 id="treasure_blueprint_catalog",
 enabled=true,
 description="Read-only DPropDataTable TreasureBoxPropDataMap -> BlueprintPath catalog and loaded generated-class probe.",
 steps={{
  kind="custom",
  name="treasure_blueprint_catalog_and_live_instances",
  class="<DPropDataTable>",
  role="treasure_actor_class_discovery",
  purpose="read_only_table_and_loaded_class_inventory",
  cadence=1,
  run=run
 }}
}
