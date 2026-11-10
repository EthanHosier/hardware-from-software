#include <cerrno>
#include <charconv>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <linux/gpio.h>
#include <poll.h>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
bool offset(const char* text,unsigned& value) {
    std::string s=text;auto [p,e]=std::from_chars(s.data(),s.data()+s.size(),value);
    return e==std::errc{} && p==s.data()+s.size();
}
int main(int argc,char** argv) {
    unsigned led=0,button=0;
    if(argc!=4 || !offset(argv[2],led) || !offset(argv[3],button) || led==button){
        std::cerr<<"usage: gpio_events CHIP LED_OFFSET BUTTON_OFFSET\n";return 2;
    }
    int chip=open(argv[1],O_RDONLY|O_CLOEXEC);
    if(chip<0){std::cerr<<std::strerror(errno)<<'\n';return 1;}
    gpio_v2_line_request out{};out.offsets[0]=led;out.num_lines=1;
    std::strcpy(out.consumer,"hfs-led");out.config.flags=GPIO_V2_LINE_FLAG_OUTPUT;
    if(ioctl(chip,GPIO_V2_GET_LINE_IOCTL,&out)<0){std::cerr<<"request LED: "<<std::strerror(errno)<<'\n';close(chip);return 1;}
    gpio_v2_line_request in{};in.offsets[0]=button;in.num_lines=1;
    std::strcpy(in.consumer,"hfs-button");
    in.config.flags=GPIO_V2_LINE_FLAG_INPUT|GPIO_V2_LINE_FLAG_EDGE_FALLING|GPIO_V2_LINE_FLAG_BIAS_DISABLED;
    if(ioctl(chip,GPIO_V2_GET_LINE_IOCTL,&in)<0){std::cerr<<"request button: "<<std::strerror(errno)<<'\n';close(out.fd);close(chip);return 1;}
    uint64_t last=0;bool light=false;unsigned presses=0;int result=0;
    while(presses<5){
        pollfd p{in.fd,POLLIN,0};int n=poll(&p,1,30000);
        if(n<0 && errno==EINTR)continue;
        if(n<=0 || (p.revents&(POLLERR|POLLHUP|POLLNVAL))){std::cerr<<"GPIO timeout/error\n";result=1;break;}
        if(!(p.revents&POLLIN))continue;
        gpio_v2_line_event event{};auto got=read(in.fd,&event,sizeof(event));
        if(got<0 && errno==EINTR)continue;
        if(got!=sizeof(event)){result=1;break;}
        if(last && event.timestamp_ns-last<20000000)continue;
        last=event.timestamp_ns; light=!light;
        gpio_v2_line_values values{};values.mask=1;values.bits=light?1:0;
        if(ioctl(out.fd,GPIO_V2_LINE_SET_VALUES_IOCTL,&values)<0){result=1;break;}
        std::cout<<"accepted edge "<<++presses<<" led="<<light<<'\n';
    }
    // This simple edge lockout is a baseline, not a stable-level debounce proof.
    gpio_v2_line_values off{};off.mask=1;ioctl(out.fd,GPIO_V2_LINE_SET_VALUES_IOCTL,&off);
    close(in.fd);close(out.fd);close(chip);return result;
}
