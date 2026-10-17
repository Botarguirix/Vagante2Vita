// @category Vagante2Vita
import ghidra.app.script.GhidraScript;
import ghidra.framework.options.Options;
import ghidra.program.model.listing.Program;

public class DisableSwitchAnalysis extends GhidraScript {
    @Override
    public void run() {
        Options analysis = currentProgram.getOptions(Program.ANALYSIS_PROPERTIES);
        analysis.setBoolean("Decompiler Switch Analysis", false);
        println("Disabled program-wide switch decompilation for focused analysis");
    }
}
