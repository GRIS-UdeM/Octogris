
firorder = 127; % must be odd
maxdistance = 1000;
distanceMultipler = 2;

samplerates = [ 32000, 44100, 48000, 88200, 96000, 176400, 192000 ];
%samplerates = [ 44100 ];
distances = linspace(0, maxdistance, maxdistance+1);

fileID = fopen('_firs.h','w');

fprintf(fileID,'const int kFirSize = %d;\n', firorder);
fprintf(fileID,'const int kMaxOffset = %d;\n', maxdistance);
fprintf(fileID,'const int kDistanceMultipler = %d;\n', distanceMultipler);
fprintf(fileID,'const int kSampleRates[%d] = { ', length(samplerates));
fprintf(fileID,'%d, ', samplerates);
fprintf(fileID,'};\n');

fprintf(fileID,'const float kFirs[%d][%d][%d] = {', length(samplerates), length(distances), firorder);

for sr = samplerates
    fprintf(fileID,'{');
    
    start_a = [];
    for distance = distances
        f = linspace(0, 1, 100);
        m = [];
        for nf = f
            fhz = nf * sr/2;
            lfhz = log(fhz);
            ldb = 1.2584*lfhz - 9.5422;
            db = -((distance * distanceMultipler)/100 * exp(ldb));
            if (db >= 0)
               m = [m 1];
            elseif (db <= -120)
               m = [m 0];
            else
               m = [m db2mag(db)];
            end
        end
        
        m2 = m(1:end-1);
        up = m2 + 0.00001;
        lo = m2 - 0.00001;
        [b, start_a, it] = fircls(firorder-1,f,m2,up,lo, 'none', start_a);
        if (distance == 0)
            start_a = [];
        end
        
        if any(isnan(b))
            fprintf('got NaN sr: %d distance: %d it: %d\n', sr, distance, it);
            return;
        end
        
        fprintf('done sr: %d distance: %d it: %d\n', sr, distance, it);
        
        fprintf(fileID,'{');
        fprintf(fileID,'%12.12g,', b);
        fprintf(fileID,'}\n,');
    end
    fprintf(fileID,'},');
end

fprintf(fileID,'};\n');
fclose(fileID);

