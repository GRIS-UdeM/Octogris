
type = 0;           % filter type (0 = fir, 1 = iir)

firorder = 127;     % filter order
iirorder = 8;
sr = 96000;         % sampling rate
distance = 981;     % in meters


f = linspace(0, 1, 100);
m = [];
start_a = [];

for nf = f
    fhz = nf * sr/2;
    lfhz = log(fhz);
    ldb = 1.2584*lfhz - 9.5422;
    db = -(distance/100 * exp(ldb));
    if (db >= 0)
       m = [m 1];
    elseif (db <= -120)
       m = [m 0];
    else
       m = [m db2mag(db)];
    end
    
end

if (type == 0)
    % http://www.mathworks.com/help/signal/ref/fircls.html
    m2 = m(1:end-1);
    up = m2 + 0.00001;
    lo = m2 - 0.00001;
    [b, start_a] = fircls(firorder-1,f,m2,up,lo,'trace', start_a);
    
    % http://www.mathworks.com/help/signal/ref/firls.html
    %b = firls(firorder,f,m);
    
    % http://www.mathworks.com/help/signal/ref/firpm.html
    %b = firpm(firorder,f,m);
    
    a = 1;
    stype = 'FIR';
else

    % http://www.mathworks.com/help/signal/ref/yulewalk.html
    [b,a] = yulewalk(iirorder,f,m);
    stype = 'Yule-Walker';
end

[h,w] = freqz(b,a,128);
%figure;
plot(w/pi,mag2db(abs(h)),f,mag2db(m),'--')
xlabel 'Radian frequency (\omega/\pi)', ylabel Magnitude
legend(stype,'Ideal'), legend boxoff
fvtool(b, a);
%length(b)
