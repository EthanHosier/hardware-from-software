# Work log

2026-11-10: User authorised complete authoring and publication using fresh-context page writers and independent semantic reviews. New project, separate private GitHub repository and public Vercel production domain. Plan expanded to 44 main lessons plus nine appendix pages. No physical board access assumed. Root owns shared examples, integration, verification and publication; coordinator dispatches one fresh writer per page and independently reviews it.

2026-11-10 progress: created private EthanHosier/hardware-from-software and new public Vercel project. All12Pico targets compile warning-clean with SDK2.3.1/ArmGNU14.2; host parser normal+ASan/UBSan and10PTY cases passed. Native LinuxArm64 CI compiles all7programs and both modules against officialPi6.12.96+rpt-rpi-v8 headers; DToverlay compiles. Linuxhost memory lab recorded128minor faults and expected childSIGSEGV. CI evidence42901505627 at9f2eb1b. These are software results, not physical device tests.

User requested immediate Vercel upload after each finished page. Publisher stages ONLY pages with an accepted independent review and matching content hashes, redirects unfinished crosslinks to a complete roadmap, checks all local links and anonymously verifies deployed page bytes. Initial reviewed0.1–0.4 published, then0.5 and0.6 accepted and progressive publication initiated.

Fresh-context writer0.5 and0.6 completed their pages. Coordinator hit account usage limit and next writer1.1 also errored for that limit (reported reset11Nov2026 00:31). Root independently completed full prose/diagram/primary-source reviews for0.5 and0.6. No further lesson writing was substituted for the requested fresh-writer/independent-review method. Continue from1.1 after agents can run.

Publication correction: Vercel blocked two interim deployments because earlier commits used the machine's auto-detected author, which had no linked Git account. Configured ONLY this repository with verified owner Ethan Hosier's GitHub noreply identity and committed reviewed Chapter0; a subsequent deployment reachedREADY. Publisher now serialises builds with a local lock, requests no-wait and checks terminal states with a bounded poll, and handles the CLI's nullable productionUrl. No old private project setting was changed.

Chapter0 public release verified: https://hardware-from-software.vercel.app, deploymentdpl_5rWPDuyPwD1BETsZGKKQKvzronaL. All6catalogpages anonymouslymatchedlocaloutput; all469local link/anchor checks passed. Browsercheckeddesktopand390pxnarrowlayout, image loads, correct19breadboardsearchresults; viewportrestored. Publicsource-downloadarchive andCSS/search/SVGresources anonymouslybytechecked too; see reviews/publication-check.json. GitHubprivacyverified. Nextpage1.1 remainsunwrittenbecausefresh-agentaccountquota, notbecausepublicationfailed.

## Resumed authoring and I²C pad audit

User requested continuation after usage became available. Coordinator and page1.1 writer resumed. Inspection of pinned SDK gpio_set_function showed it preserves pad pulls; reset RP2350 pads have pull-down enabled. Explicitly disabled internal pulls on GP4/GP5 in both I²C examples so the intended external board pull-ups determine the bus bias. Both affected firmware targets recompiled and linked successfully; physical bus testing remains unexercised.

Publication staging now requires hash coverage for all actual lesson includes and SVG references and copies only reviewed SVGs plus the theme stylesheet. Six-page build/link checks pass. CI run42904806012 passed Linux host/parser/serial/memory checks and nativeArm64 applications, Pi-header module and overlay compilation at70a2d49.

## Public repository authorised

User explicitly authorised making the GitHub repository public after seeing an earlier Vercel Hermes-author notification. Changed EthanHosier/hardware-from-software visibility to PUBLIC with GitHubCLI; unauthenticated GitHubAPI returned200 and private:false. Recent Vercel production deployments already succeed using the repository-local owner commit identity. Pages1.1–1.4 and2.1–2.2 are now independently reviewed and published; each acceptance triggered its own successful production release and anonymous exact-output check. The2.1 nested-include rendering failure was repaired by its same writer and rechecked before publication.

Chapters0–3 now fully independently reviewed and published,17of53catalog pages. Root independently reviewed alternating2.2/2.4/3.2 while coordinator reviewed the others. Download archive now has its own README and only relevant setup/build/test scripts; isolated extracted-bundle host compilation and normal/sanitized parser checks pass. Broad anonymous live check verified46resources at15-page release; every later release independently checks allchapterbytes/manifest. Button polling contract clarified as observed-transition output, not complete electrical bounce capture; unchanged firmware.

Kernel error-unwind audit: if misc_register fails after IRQ setup, stop IRQ/work and explicitly drive the LED low before dropping the state reference. This now matches normal removal quiescence; native Pi-header CI compilation requested for changed kernel source. No published lesson includes this source yet.

CI42910194223 succeeded at544c55d: both Linuxhost and nativeArm64/Pi-header module+overlay jobs passed after the kernel failure-unwind change. Updated source hash/evidence; no hardware runtime implied.
