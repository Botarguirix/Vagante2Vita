// Retry only failed functions without repeating program analysis.
// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.Function;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.io.*;
import java.util.*;

public class RetryDecomp extends GhidraScript {
    public void run() throws Exception {
        String[] args=getScriptArgs();Path root=Paths.get(args[0]);
        int timeout=args.length>1?Integer.parseInt(args[1]):120;
        List<String> lines=Files.readAllLines(root.resolve("functions.tsv"),StandardCharsets.UTF_8);
        DecompInterface d=new DecompInterface();if(!d.openProgram(currentProgram))throw new IOException("Cannot open program");
        int recovered=0,failed=0;
        try(BufferedWriter log=Files.newBufferedWriter(root.resolve("retry.tsv"),StandardCharsets.UTF_8)) {
            log.write("address\tname\tstatus\tdetail\n");
            for(int i=1;i<lines.size() && !monitor.isCancelled();i++) {
                String[] row=lines.get(i).split("\t",-1);if(!row[2].equals("failed"))continue;
                Function f=currentProgram.getFunctionManager().getFunctionAt(toAddr(row[0]));
                if(f==null){failed++;log.write(row[0]+"\t"+row[1]+"\tfailed\tmissing function\n");continue;}
                DecompileResults r=d.decompileFunction(f,timeout,monitor);
                if(r.decompileCompleted() && r.getDecompiledFunction()!=null) {
                    String filename=row[0]+".c";
                    Files.writeString(root.resolve(filename),"/* Ghidra pseudocode: "+row[1]+" @ "+row[0]+". Not verified recompilable source. */\n"+r.getDecompiledFunction().getC(),StandardCharsets.UTF_8);
                    lines.set(i,row[0]+"\t"+row[1]+"\tok\t"+filename);recovered++;
                    log.write(row[0]+"\t"+row[1]+"\tok\t\n");
                }else{failed++;log.write(row[0]+"\t"+row[1]+"\tfailed\t"+r.getErrorMessage().replace('\n',' ').replace('\t',' ')+"\n");}
                log.flush();println("Retry "+row[1]+": recovered="+recovered+", failed="+failed);
            }
        }finally{d.dispose();}
        Files.write(root.resolve("functions.tsv"),lines,StandardCharsets.UTF_8);
        Files.writeString(root.resolve("retry_summary.txt"),"recovered="+recovered+"\nfailed="+failed+"\n",StandardCharsets.UTF_8);
    }
}
