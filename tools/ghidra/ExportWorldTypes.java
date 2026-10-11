// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.program.model.data.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.util.Iterator;
public class ExportWorldTypes extends GhidraScript {
 public void run() throws Exception {
  Path output=Paths.get(getScriptArgs()[0]);Files.createDirectories(output);
  StringBuilder enums=new StringBuilder("owner\tname\tvalue\n");
  Iterator<DataType> types=currentProgram.getDataTypeManager().getAllDataTypes();
  while(types.hasNext()){
   DataType t=types.next();
   if(t instanceof ghidra.program.model.data.Enum){
    ghidra.program.model.data.Enum e=(ghidra.program.model.data.Enum)t;
    for(String name:e.getNames())enums.append(t.getPathName()).append("\t").append(name).append("\t").append(e.getValue(name)).append("\n");
   }
  }
  Files.writeString(output.resolve("enums.tsv"),enums.toString(),StandardCharsets.UTF_8);
  println("Exported enum members to "+output);
 }
}
