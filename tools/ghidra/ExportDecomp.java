// Export Ghidra pseudocode locally; it is not portable recovered source.
// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;

public class ExportDecomp extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();
        if(args.length<1) throw new IllegalArgumentException("output-directory [max-functions]");
        Path root=Paths.get(args[0]);Files.createDirectories(root);
        int limit=args.length>1?Integer.parseInt(args[1]):0;
        DecompInterface decompiler=new DecompInterface();
        if(!decompiler.openProgram(currentProgram)) throw new IOException("Cannot open program");
        int ok=0,fail=0,total=0;
        try (BufferedWriter index=Files.newBufferedWriter(root.resolve("functions.tsv"),StandardCharsets.UTF_8)) {
            index.write("address\tname\tstatus\tfile\n");
            FunctionIterator it=currentProgram.getFunctionManager().getFunctions(true);
            while(it.hasNext() && !monitor.isCancelled() && (limit==0 || total<limit)) {
                Function f=it.next();if(f.isExternal() || f.isThunk())continue;
                total++;
                String addr=f.getEntryPoint().toString();String name=f.getName(true);
                DecompileResults result=decompiler.decompileFunction(f,30,monitor);
                if(result.decompileCompleted() && result.getDecompiledFunction()!=null) {
                    String filename=addr+".c";
                    Files.writeString(root.resolve(filename),"/* Ghidra pseudocode: "+name+" @ "+addr+". Not verified recompilable source. */\n"+result.getDecompiledFunction().getC(),StandardCharsets.UTF_8);
                    index.write(addr+"\t"+name.replace('\t',' ')+"\tok\t"+filename+"\n");ok++;
                } else {index.write(addr+"\t"+name.replace('\t',' ')+"\tfailed\t\n");fail++;}
                if(total%100==0) {println("Exported "+ok+"; failed "+fail);index.flush();}
            }
        } finally {decompiler.dispose();}
        Files.writeString(root.resolve("summary.txt"),"attempted="+total+"\nsucceeded="+ok+"\nfailed="+fail+"\n",StandardCharsets.UTF_8);
        println("Export complete: "+ok+" functions, "+fail+" failures");
    }
}
