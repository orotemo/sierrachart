#include "sierrachart.h"

SCDLLName("Frozen Tundra Discord Room Studies")

/*
    Written by Malykubo and Frozen Tundra in FatCat's Discord Room
    Modified to add adaptive multiplier based on visible price range
*/

SCSFExport scsf_ChangeVolAtPriceMult(SCStudyInterfaceRef sc)
{
    SCInputRef DebugOn = sc.Input[0];
    SCInputRef LookbackInterval = sc.Input[2];
    SCInputRef MagicNumber = sc.Input[3];
    SCInputRef UseBarAverageInstead = sc.Input[4];
    SCInputRef MinimalVapPercentageChangeThreshold = sc.Input[5];
    SCInputRef UseAdaptiveMultiplier = sc.Input[6];
    SCInputRef TargetLevelsCount = sc.Input[7];
    // Add inputs to track last state
    SCInputRef LastFirstBar = sc.Input[8];
    SCInputRef LastLastBar = sc.Input[9];
    
    SCString log_message;
    
    // Configuration
    if (sc.SetDefaults)
    {
        sc.GraphRegion = 0;
        sc.ValueFormat = VALUEFORMAT_INHERITED;
        sc.ScaleRangeType = SCALE_SAMEASREGION;
        sc.DrawZeros = 0;
        
        DebugOn.Name = "Debug Enabled?";
        DebugOn.SetYesNo(0);
        LookbackInterval.Name = "Lookback Interval # Bars";
        LookbackInterval.SetInt(sc.IndexOfLastVisibleBar - sc.IndexOfFirstVisibleBar);
        MagicNumber.Name = "Magic Multiplier (ticks)";
        MagicNumber.SetFloat(0.3);
        UseBarAverageInstead.SetYesNo(0);
        UseBarAverageInstead.Name = "Use bar averages instead of max visible bar for diff calculation";
        MinimalVapPercentageChangeThreshold.Name = "Percentage that VAP must change to trigger re-calc";
        MinimalVapPercentageChangeThreshold.SetFloat(0.30);
        
        UseAdaptiveMultiplier.Name = "Use Adaptive Multiplier";
        UseAdaptiveMultiplier.SetYesNo(1);
        TargetLevelsCount.Name = "Target Number of Price Levels";
        TargetLevelsCount.SetInt(50);
        
        // Hidden state tracking inputs
        LastFirstBar.Name = "LastFirstBar";
        LastFirstBar.SetInt(-1);
        LastLastBar.Name = "LastLastBar";
        LastLastBar.SetInt(-1);
        
        return;
    }
    
    if (sc.LastCallToFunction)
        return;
    
    // Only proceed if chart is active
    if (!sc.ChartWindowIsActive)
        return;
        
    int firstVisibleBar = sc.IndexOfFirstVisibleBar;
    int lastVisibleBar = sc.IndexOfLastVisibleBar;
    
    // Check if visible range has changed
    if (firstVisibleBar == LastFirstBar.GetInt() && lastVisibleBar == LastLastBar.GetInt())
        return;  // No change in visible range, exit early
        
    // Update stored range
    LastFirstBar.SetInt(firstVisibleBar);
    LastLastBar.SetInt(lastVisibleBar);
    
    if (DebugOn.GetInt() == 1) {
        log_message.Format("Visible range changed - Index: %d, FirstVisible: %d, LastVisible: %d", 
                         sc.Index, firstVisibleBar, lastVisibleBar);
        sc.AddMessageToLog(log_message, 1);
    }
    
    // Calculate price range across visible bars
    float highestPrice = sc.BaseData[SC_HIGH][firstVisibleBar];
    float lowestPrice = sc.BaseData[SC_LOW][firstVisibleBar];
    
    // Look through all visible bars
    for (int i = firstVisibleBar; i <= lastVisibleBar; i++) {
        if (i < 0) continue;
        highestPrice = max(highestPrice, sc.BaseData[SC_HIGH][i]);
        lowestPrice = min(lowestPrice, sc.BaseData[SC_LOW][i]);
    }
    
    int vap;
    
    if (UseAdaptiveMultiplier.GetYesNo()) {
        float priceRange = highestPrice - lowestPrice;
        float ticksInRange = priceRange / sc.TickSize;
        vap = sc.Round(ticksInRange / TargetLevelsCount.GetInt());
        vap = max(vap, 1);
        
        if (DebugOn.GetInt() == 1) {
            log_message.Format("Visible bars: %d to %d, Price Range: %f, Ticks: %f, Calculated VAP: %d", 
                             firstVisibleBar, lastVisibleBar, priceRange, ticksInRange, vap);
            sc.AddMessageToLog(log_message, 1);
        }
    } else {
        // Original VAP calculation
        float max_bar_diff = 0;
        float bar_diff_sums = 0;
        int lookback_interval = LookbackInterval.GetInt();
        
        for (int i = lastVisibleBar - lookback_interval; i <= lastVisibleBar; i++) {
            if (i < 0) continue;
            float tmp_bar_diff = sc.BaseData[SC_HIGH][i] - sc.BaseData[SC_LOW][i];
            max_bar_diff = max(max_bar_diff, tmp_bar_diff);
            bar_diff_sums += tmp_bar_diff;
        }
        
        float magic_number = MagicNumber.GetFloat();
        vap = sc.Round(max_bar_diff / magic_number);
        if (UseBarAverageInstead.GetInt() == 1) {
            vap = sc.Round((bar_diff_sums/lookback_interval) / magic_number);
        }
        
        if (sc.TickSize > 0.01) {
            vap = sc.Round(max_bar_diff * magic_number * sc.TickSize);
        }
        
        if (vap == 0) {
            vap = 1;
        }
    }
    
    // Update VAP if it's different
    if (sc.VolumeAtPriceMultiplier != vap) {
        if (DebugOn.GetInt() == 1) {
            log_message.Format("Updating VAP: old=%d, new=%d", 
                             sc.VolumeAtPriceMultiplier, vap);
            sc.AddMessageToLog(log_message, 1);
        }
        
        sc.VolumeAtPriceMultiplier = vap;
    }
}
